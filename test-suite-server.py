import json
import random
import socket
import subprocess
import sys
import time
import traceback

import analyze_pcap

std_ports = {
    "ctrl": 6000,
    "udp_punch": 6001
}
MAX_CTRL_MSG_SIZE = 1024 * 1024
run_time=30
res_dir="results"

server_config = {
    "available_cong_algs": ["copa", "cubic", "bbr"]
}

cong_algs = {
    "cubic" : {"prog": "iperf3", "port": "5201", "time": str(run_time)},
    "bbr" : {"prog": "iperf3", "port": "5201", "time": str(run_time)},
    "copa" : {"prog": "receiver", "time": str(run_time+5)}
}

# Ctrl socket
ctrl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
ctrl_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
ctrl_sock.bind(("0.0.0.0", std_ports["ctrl"]))
ctrl_sock.listen(10)
while True:
    try:
        print("Waiting for connection")
        ctrl_conn, ctrl_client_addr = ctrl_sock.accept()
        conn_id = str(random.randint(0, 1e9))
        print("Connected to client '%s'. Assigned id '%s'" % (str(ctrl_client_addr), conn_id))
        while True:
            request_str = ctrl_conn.recv(MAX_CTRL_MSG_SIZE)
            if len(request_str) == 0: continue
            print("Got", request_str)
            request = json.loads(request_str)
            if request["request"] == "get_config":
                config = server_config
                config["conn_id"] = conn_id
                ctrl_conn.sendall(json.dumps(server_config))

            elif request["request"] == "test_cong_alg":
                if request["cong_alg"] in ["cubic", "bbr"]:
                    if request["cong_alg"] == "cubic":
                        subprocess.call(["sudo", "sysctl", "-w", "net.ipv4.tcp_congestion_control=cubic"])
                        subprocess.call(["sudo", "sysctl", "-w", "net.core.default_qdisc=pfifo_fast"])
                    elif request["cong_alg"] == "bbr":
                        subprocess.call(["sudo", "sysctl", "-w", "net.ipv4.tcp_congestion_control=bbr"])
                        subprocess.call(["sudo", "sysctl", "-w", "net.core.default_qdisc=fq"])
                    else:
                        assert(False)
                    subprocess.Popen(('sudo tcpdump -i eth0 -w %s/%s-%s-%s' % (res_dir, ctrl_client_addr[0], conn_id, request["cong_alg"])).split(' '))
                    ctrl_conn.sendall(json.dumps(cong_algs[request["cong_alg"]]))

                    subprocess.call(["iperf3", "-s", "--one-off"])
                    subprocess.call(['sudo', 'pkill', 'tcpdump'])
                    subprocess.call(['sudo', 'pkill', 'tcpdump'])
                elif request["cong_alg"] in ["copa"]:
                    subprocess.call(["sudo", "sysctl", "-w", "net.core.default_qdisc=pfifo_fast"])
                    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                    udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                    udp_sock.bind(("", std_ports["udp_punch"]))

                    ctrl_conn.sendall(json.dumps(cong_algs[request["cong_alg"]]))

                    _, addr = udp_sock.recvfrom(1024)
                    print("UDP punched %s" % str(addr))
                    udp_sock.sendto("Sesame", addr)
                    udp_sock.close()

                    time.sleep(5) # Give the client time to set things up
                    subprocess.Popen(('sudo tcpdump -i eth0 -w %s/%s-%s-%s' % (res_dir, ctrl_client_addr[0], conn_id, request["cong_alg"])).split(' '))

                    subprocess.call(["./sender",
                                     "serverip="+addr[0],
                                     "serverport="+str(addr[1]),
                                     "sourceport="+str(std_ports["udp_punch"]),
                                     "onduration="+str(int(run_time * 1000)),
                                     "cctype=markovian",
                                     "delta_conf=auto",
                                     "offduration=0",
                                     "traffic_params=deterministic,num_cycles=1"])
                    print("Copa done")
                    subprocess.call(['sudo', 'pkill', 'tcpdump'])
                    subprocess.call(['sudo', 'pkill', 'tcpdump'])
                else:
                    print("Request for unrecognized algorithm '%s'" % request["cong_alg"])

            elif request["request"] == "close":
                print("Closing connection with client '%s'" % str(ctrl_client_addr))
                ctrl_conn.close()
                break

            results = analyze_pcap.parse_file('%s/%s-%s-%s' % (res_dir, ctrl_client_addr[0], conn_id, request["cong_alg"]))
            print(ctrl_client_addr[0], conn_id, request["cong_alg"], results)
    except Exception:
        print("Error while communicating with client.")
        print(traceback.format_exc())
