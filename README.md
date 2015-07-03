# genericCC
An interface to program any congestion control protocol for an unreliable connection based protocol sent over UDP. It comes with a clean TrafficGenerator interface that can generate traffic for each of these various protocols. Also supports other congestion control protocols (refer Information section below)

Installation
------------
Run: 'makepp' in the base directory. 
This requires makepp (short for "make plus plus"). On ubuntu, install by running "sudo apt-get install makepp" on the terminal.

Run the congestion control algorithm of your choice by reading the Information section below

Information
-----------
The current version implements remy, kernel cc (cubic on linux), UDT's TCP aimd implementation and PCC (slightly doubtful and buggy) on a poisson on-off traffic model (currently configures for 5s mean).

For remy and aimd congestion control, it is not a reliable transport. Kernel's and UDT's tcp implementations are naturally reliable. This could cause bias in readings.

It sends it on a real network. It prints command line usage except for cctype, which is assumed to be 'remy' by default (other options are 'kernel', 'tcp' and 'pcc').

Further, the signal type (with loss signal-without slow_rewma, without slow-rewma and aith slow_rewma-without loss signal) are controlled by which memory-xx.hh/memory-xx.cc are included (xx can be 'default,' 'with-loss-signal' and 'without-slow-rewma') and also by the appropriate protobufs-xx folder. This are controlled by the options in configs.hh and by the INCLUDES variable in makefile. According to that appropriate binary (sender) is generated. Note that only 'memory-default' is in active development for now, so the others may be buggy (although efforts are taken to ensure that this is not the case).

Receivers:
 - The receivers for various types of congestion control algorithms are as follows
   - 'remy', 'tcp' - receiver (simply echoes back the first few bytes (ie. the header) if the received UDP packet)
   - 'kernel' - 'iperf -s' (which is required on both ends, for measuring RTT, use 'tcpdump' followed by 'tcptrace -lr'
   - 'pcc' - pcc-receiver

Can be run either on a real network or on mahi-mahi. When running on mahi-mahi, note that the receiver's address is the address of the interface on the outside part of the os (do an ifconfig on a fresh terminal). Everything except the kernel version (which uses iperf) runs over UDP, last time I could not send udp between two GENI nodes, although it should be possible to do so.

PS: sockperf can also be used instead of iperf, just uncomment the apropriate line and comment the iperf line in kernelTCP.hh​

PPS: support for 'pcc' has been temporarily removed due to fears of a bug. I do plan to put it back soon.