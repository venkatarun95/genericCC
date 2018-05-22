# genericCC

An interface to program congestion control protocols transmitted sent
over UDP. It comes with a clean TrafficGenerator interface that can
generate traffic for each of these various protocols. Also supports
other congestion control protocols (refer Information section below)

The current version implements Copa, Remy, Kernel CC (Cubic on linux),
UDT's TCP AIMD implementation and PCC (deprecated). The traffic can be
either deterministic or poisson on-off where on period can be
specified in seconds or bytes.

Note: transport for Copa, Remy and AIMD is not reliable whereas Kernel
TCP (run using iperf) is reliable. None of these are capable of
transporting data yet, though such a version is in development in the
'full_tcp' branch.

Installation
------------

Dependencies: Makepp, Google protocol buffers, Boost C+ library and
jemalloc. On Ubuntu install using

`sudo apt-get install g++ makepp libboost-dev libprotobuf-dev protobuf-compiler libjemalloc-dev iperf libboost-python-dev`

Run: `makepp` in the base directory. It should create 'sender' and
'receiver' executables in the home directory. Uses the makepp build
system (short for "make plus plus"). 'iperf' is required for running
kernel TCP.

Usage
-----------

The executable `./sender` should be run on the machine from which data
is to be transmitted. `./receiver` should be run on the receiver
side. 'receiver' is a simple program that listens on port 8888. It
acks every packet it gets and does not maintain any state
information. All intelligence is on the sender's side and hence all
options are also specified here.

### Congestion Control

The congestion control algorithm is chosen using the
'cctype=[remy|markovian|kernel|tcp]'. 'markovian' denotes Copa,
'kernel' denotes the kernel's default tcp run using iperf and tcp
denotes a simple AIMD algorithm. If no algorithm is specified, Remy is
used by default. If Remy is used, the rat file should be specified
using 'if=filepath'. The delta configuration for markovian can be
specified using 'delta_conf'. For instance
'delta_conf=constant_delta:1' will make Copa use a constant delta of
1.0. 

Various variants of Adaptive Copa can be specified. For instance
'bounded_delay_end:100 will bound the end-to-end delay to
100ms. Similarly 'bounded_percentile_delay_end' can also be used,
which will bound the 95th percentile of end-to-end delay. Copa, Remy
and AIMD TCP expect a 'MIN_RTT' environment variable to give an
estimate in ms (only Copa uses this value though). If this value is
not known, a sufficiently large value can be given and the sender will
calculate the minimum it sees (eg. `export MIN_RTT=10000`). To create
a 'maximize throughput' Adaptive Copa, simply set the delay bound to
be very large.

### Traffic

By default the sender repeatedly switches on and off with the on and
off periods being drawn from a possion distribution with mean 5s. The
mean can be set using 'onduration=' and 'offduration=', specified in
milliseconds or bytes.

traffic_params: 'traffic_params=*option1,option2,...*' can be used to
control several characteristics of the traffic
generator. 'deterministic' makes the on and off periods always equal
the specified values in 'onduration' and
'offduration'. 'byte_switched' causes 'onduration' to be interpreted
and the average number of bytes to send rather than the average number
of milliseconds to switch on for. 'num_cycles' specifies the number of
on-off cycles. By default it cycles an infinite number of times.

Examples:

  * 'onduration=1000 offduration=1000': Exponentially distributed
on-off times with 1s mean.

  * 'onduration=1000000 traffic_params=byte_switched': Transmit 1
Mbyte on average and switch off for 5s on average (both are
exponentially distributed).  

  * 'onduration=10000 offduration=0
traffic_params=deterministic,num_cycles=1': Switches on for 10 seconds
and exits.

### Network Addressing

The receiver listens on port 8888. The sender requires 'serverip' and
'sourceip' (source IP is required because of clumsy coding in
UDPSocket class, it should be easy to modify this class to remove this
requirement). 'sourceport' and 'serverport' can be optionally
specified. By default 'serverport' is assumed to be 8888 and
sourceport is some available port allocated by the OS.

### Receiver

All algorithms except 'kernel' use the 'receiver' executable included
created in this repository. 'kernel' uses iperf, so `iperf -s` must be
run on the receiver side.



### Miscellaneous

'sockperf' can also be used instead of 'iperf', just uncomment the
apropriate line and comment the iperf line in kernelTCP.hh​

Support for 'pcc' was present byt has been temporarily removed due to
fears of a bug. Also AIMD TCP has not been well tested.

NOTE: I have not tested the following in a long time:- The signal type
(with loss signal-without slow_rewma, without slow-rewma and with
slow_rewma-without loss signal) are controlled by which
memory-xx.hh/memory-xx.cc are included (xx can be 'default,'
'with-loss-signal' and 'without-slow-rewma') and also by the
appropriate protobufs-xx folder. This are controlled by the options in
configs.hh and by the INCLUDES variable in makefile. According to that
appropriate binary (sender) is generated. Note that only
'memory-default' is in active development for now, so the others may
be buggy (although efforts are taken to ensure that this is not the
case).

Can be run either on a real network or on mahi-mahi. When running on
mahi-mahi, note that the receiver's address is the address of the
interface on the outside part of the os (do an ifconfig on a fresh
terminal). Everything except the 'kernel' tcp (which uses iperf) runs
over UDP.

### Example

Running on Mahimahi (see [mahimahi.mit.edu](http://mahimahi.mit.edu))

`sudo sysctl -w net.ipv4.ip_forward=1`

Start two nested mahimahi shells emulating a 12Mbps link with a 100ms
delay as follows (assuming you have the relevant trace file, for
12Mbps, it is just a text file containing the single ascii character
'1'):

`mm-delay 50 mm-link trace-12Mbps trace-12Mbps`

Find the ip addresses inside and outside the mahimahi shell by running
`ifconfig` inside and outside the shell resp. Inside the shell look for an
'ingress' interface and outside the shell, look for a 'delay-*' interface. For
instance they could be '100.64.0.1' and '100.64.0.4' resp. The external address
is also available in the variable `MAHIMAHI_BASE`. Run `./receiver` outside the
mahimahi shells. Run the following command inside the shells to start a sender
that uses Adaptive Copa that bounds end-to-end delay to 110ms. It will transmit
in 10s bunches. You can run several such sender processes in parallel, but
running more than n-1 senders (where n is the number of available cores) is not
recommended, especially for protocols that could be potentially pacing sensitive
such as Remy.

`./sender serverip=$MAHIMAHI_BASE offduration=0 onduration=60000
cctype=markovian delta_conf=do_ss:auto:0.5
traffic_params=deterministic,num_cycles=1`

