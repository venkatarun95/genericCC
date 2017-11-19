import dpkt
import re
import socket
import sys

def parse_file(filename):
    re_filename = re.compile(".*/(?P<ip>[\d.]*)-\d*-(?P<congalg>\w*)")
    mtch_filename = re_filename.match(filename)
    client_ip = mtch_filename.group('ip')
    congalg = mtch_filename.group('congalg')
    pcap = dpkt.pcap.Reader(open(filename, 'r'))

    start_time, end_time = -1, -1
    num_bytes = 0
    rtt_sum, num_rtt_samples = 0.0, 0
    sent_time = {}
    for ts, buf in pcap:
        eth = dpkt.ethernet.Ethernet(buf)
        if type(eth.data) == str:
            continue
        if type(eth.data) != dpkt.ip.IP:
            continue
        if client_ip not in [socket.inet_ntoa(eth.data.dst), socket.inet_ntoa(eth.data.src)]:
            continue

        if congalg in ["cubic", "bbr"]:
            if type(eth.data.data) != dpkt.tcp.TCP:
                continue
            if 5201 not in [eth.data.data.sport, eth.data.data.dport]:
                continue
            if eth.data.data.sport == 5201:
                direction = "data"
                seq = eth.data.data.seq
            else:
                direction = "ack"
                seq = eth.data.data.ack
        elif congalg in ["copa"]:
            if type(eth.data.data) != dpkt.udp.UDP:
                continue
            if 8888 not in [eth.data.data.sport, eth.data.data.dport]:
                continue
            if eth.data.data.sport == 8888:
                direction = "ack"
                seq = eth.data.data.data[:8]
            else:
                direction = "data"
                seq = eth.data.data.data[:8]
        else:
            print("Unrecognized congalg")
            exit(1)
        # if start_time == -1:
        #     start_time = ts
        # print (socket.inet_ntoa(eth.data.src), socket.inet_ntoa(eth.data.dst), eth.data.data.seq, eth.data.data.ack, ts - start_time)
        # continue
        #print(direction, seq)

        if direction == "data":
            if start_time == -1:
                start_time = ts
            end_time = ts
            num_bytes += len(buf)
            if seq not in sent_time:
                sent_time[seq] = ts
            else:
                # Don't want to measure on retransmitted packets
                sent_time[seq] = -1
        else:
            assert(direction == "ack")
            if seq in sent_time and sent_time[seq] != -1:
                rtt_sum += ts - sent_time[seq]
                num_rtt_samples += 1

    tpt = 8e-6 * num_bytes / (end_time - start_time)
    rtt = rtt_sum / num_rtt_samples
    return (tpt, rtt, end_time - start_time)

if __name__ == "__main__":
    for fname in sys.argv[1:]:
        print(fname)
        print(parse_file(fname))

