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

echo "Usage: source_interface serverip serverport cctype ratname delta numsenders linkppt numkernels outfilename"

source_interface=$1
sourceip=`ip addr show $source_interface | sed -n '/inet /{s/^.*inet \([0-9.]\+\).*$/\1/;p}'`
serverip=$2
serverport=$3
cctype=$4
ratname=$5
delta=$6
num_senders=$7
linkppt=$8
numkernels=$9
outfilename=${10}

remytype=
onduration=5000
offduration=5000
echo "Running for 5000ms"

echo "Running for $num_senders sender(s)"
echo "prober $serverip $sourceip $serverport $outfilename-prober"
pids=""
kernelpids=""
proberpid=

if [ $cctype = "kernel" ]
then
	sudo tcpdump -i $source_interface -w /tmp/tcpdump.dat & 
elif [ $numkernels>0 ]
then
	echo "Instantiating $numkernels kernels"
	sudo tcpdump -i $source_interface -w /tmp/controltcpdump.dat & 

	for (( i=1; i<=$numkernels; i++ ))
	do
		#iperf -c $serverip -t 100 & pid=$!
		kernelpids+=" $pid"
		echo "Starting kernel onoff sender"
		./sender$remytype cctype=kernel serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration linkrate=$linkppt >> $outfilename & pid=$!
	done
fi

./prober $serverip $sourceip $serverport $outfilename-prober & proberpid=$!

for (( i=1; i<=$num_senders; i++ ))
do	
	./sender$remytype cctype=$cctype if=$ratname delta=$delta serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration linkrate=$linkppt >> $outfilename & pid=$!
	#echo "cctype=$cctype if=$ratname serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=$onduration offduration=$offduration"
	pids+=" $pid"
done

sleep 100
#echo "Warning: Running for only 10s"
kill -9 $pids
kill -9 $kernelpids
kill -9 $proberpid
killall -9 sender$remytype

if [ $cctype = "kernel" ]
then
	sudo killall -9 tcpdump
	tcptrace -lr /tmp/tcpdump.dat >> $outfilename-tcptrace
	echo '--------------END OF RUN--------------' >> $outfilename-tcptrace
else
	sudo killall -9 tcpdump
	tcptrace -lr /tmp/controltcpdump.dat >> $outfilename-competing-tcptrace
	echo '--------------END OF RUN--------------' >> $outfilename-competing-tcptrace
fi

echo '--------------END OF RUN--------------' >> $outfilename

# tcptrace -lr tcpdump-senders16 | grep --regex='\(RTT avg\)\|\(throughput\)' > tcptrace-senders16