#!/bin/bash

kill_child_processes() {
    isTopmost=$1
    curPid=$2
    childPids=`ps -o pid --no-headers --ppid ${curPid}`
    for childPid in $childPids
    do
        kill_child_processes 0 $childPid
    done
    if [ $isTopmost -eq 0 ]; then
        kill -9 $curPid 2> /dev/null
    fi
}

# Ctrl-C trap. Catches INT signal
trap "kill_child_processes 1 $$; exit 0" INT

sourceip=`ip addr show ingress | sed -n '/inet /{s/^.*inet \([0-9.]\+\).*$/\1/;p}'`
serverip=100.64.0.1
serverport=5001

cctype=kernel
ratname=rats/fig4-rtt/145--155.dna.4 #fig6-topology/bigbertha-10x.dna.4 #fig1-calibration/150-alone.dna.2 
remytype= 

onduration=1000
offduration=1000

for num_senders in 2 #1 4 8 12 16
do
	echo "Running for $num_senders sender(s)"
	pids=""

	if [ $cctype = "kernel" ]
	then
		sudo tcpdump -i ingress -w tcpdump.dat  &
	fi

	for (( i=1; i<=$num_senders; i++ ))
	do	
		./sender$remytype if=$ratname cctype=$cctype serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration >> range-test-results/$cctype-link$1-rtt$2-numsenders$num_senders & pid=$!
		pids+=" $pid"
	done

	#trap 'kill -9 $pids && killall -9 sender$remytype' EXIT

	sleep 100
	echo "Warning: Running for only 100s"
	kill -9 $pids
	
	if [ $cctype = "kernel" ]
	then
		sudo killall -9 tcpdump
		tcptrace -lr tcpdump.dat > range-test-results/kernel-senders$num_senders
	fi
	
	killall -9 sender$remytype
done

# tcptrace -lr tcpdump-senders16 | grep --regex='\(RTT avg\)\|\(throughput\)' > tcptrace-senders16