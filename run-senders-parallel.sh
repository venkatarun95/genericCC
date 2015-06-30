#!/bin/bash

# Sample run: ./sender serverip=127.0.0.1 if=./input-rats/150-alone.dna.2 sourceip=127.0.0.1 serverport=8888 sourceport=0 offduration=5000 onduration=5000 cctype=remy linkrate=1000

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

#sourceip=`ip addr show ingress | sed -n '/inet /{s/^.*inet \([0-9.]\+\).*$/\1/;p}'`
#serverip=100.64.0.1
#serverport=5001

#cctype=kernel
#ratname=rats/fig4-rtt/145--155.dna.4 #fig6-topology/bigbertha-10x.dna.4 #fig1-calibration/150-alone.dna.2 
#remytype= 

#onduration=1000
#offduration=1000

echo "Usage: source_interface serverip serverport cctype ratname numsenders linkppt outfilename"

source_interface=$1
sourceip=`ip addr show $source_interface | sed -n '/inet /{s/^.*inet \([0-9.]\+\).*$/\1/;p}'`
serverip=$2
serverport=$3
cctype=$4
ratname=$5
num_senders=$6
linkppt=$7
outfilename=$8

remytype=
onduration=5000
offduration=5000

#for num_senders in 2 #1 4 8 12 16
#do
	echo "Running for $num_senders sender(s)"
	pids=""

	if [ $cctype = "kernel" ]
	then
		sudo tcpdump -i $source_interface -w /tmp/tcpdump.dat  &
	fi

	for (( i=1; i<=$num_senders; i++ ))
	do	
		./sender$remytype cctype=$cctype if=$ratname serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration linkrate=$linkppt >> $outfilename & pid=$!
		#echo "cctype=$cctype if=$ratname serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration"
		pids+=" $pid"
	done

	#trap 'kill -9 $pids && killall -9 sender$remytype' EXIT

	sleep 100
	#echo "Warning: Running for only 10s"
	kill -9 $pids
	killall -9 sender$remytype
	
	if [ $cctype = "kernel" ]
	then
		sudo killall -9 tcpdump
		tcptrace -lr /tmp/tcpdump.dat > $outfilename-tcptrace
		echo '--------------END OF RUN--------------' >> $outfilename-tcptrace
	fi

	echo '--------------END OF RUN--------------' >> $outfilename
#done

# tcptrace -lr tcpdump-senders16 | grep --regex='\(RTT avg\)\|\(throughput\)' > tcptrace-senders16