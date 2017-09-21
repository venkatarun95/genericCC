import json
import socket
import subprocess
import sys
import time

std_ports = {
    "ctrl": 6000,
    "udp_punch": 6001
}
MAX_CTRL_MSG_SIZE = 1024 * 1024

server_ip = sys.argv[1]
genericcc_port = 8888

ctrl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ctrl_sock.connect((server_ip, std_ports["ctrl"]))
ctrl_sock.sendall('{"request": "get_config"}')
server_configs = json.loads(ctrl_sock.recv(MAX_CTRL_MSG_SIZE))
print("Connection id assigned by server: %s" % server_configs["conn_id"])

for congalg in server_configs["available_cong_algs"]:
    print("Testing '%s'" % congalg)
    ctrl_sock.sendall('{"request": "test_cong_alg", "cong_alg" : "%s"}' % congalg)
    run_params = json.loads(ctrl_sock.recv(MAX_CTRL_MSG_SIZE))
    print(run_params)
    if run_params["prog"] == "iperf3":
        time.sleep(5) # Give the server some time to set things up
        print((["iperf3", "-c", server_ip, "-p", run_params["port"], "-t", run_params["time"], "-R"]))
        subprocess.call(["iperf3", "-c", server_ip, "-p", run_params["port"], "-t", run_params["time"], "-R"])

    elif run_params["prog"] == "receiver":
        udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        udp_sock.bind(("", 8888))
        udp_sock.settimeout(1)
        num_tries = 0
        while True:
            udp_sock.sendto("Open", (server_ip, std_ports["udp_punch"]))
            num_tries += 1
            try:
                udp_sock.recv(1024)
            except socket.timeout:
                if num_tries > 5:
                    print("Couldn't UDP punch. Aborting experiment")
                    num_tries = -1
                    break
                print("Punch failed. Retrying")
                continue
            break
        udp_sock.close()
        if num_tries == -1:
            break
        print("Punch succeeded")

        subprocess.Popen(["./receiver"])
        time.sleep(float(run_params["time"]))
        subprocess.call(["pkill", "receiver"])
    else:
        print("Unknown program '%s' for running congestion control protocol '%s'" % (run_params["prog"], congalg))

ctrl_sock.sendall('{"request": "close"}')
ctrl_sock.close()
