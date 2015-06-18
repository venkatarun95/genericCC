#!/bin/bash

sourceip=`ip addr show ingress | sed -n '/inet /{s/^.*inet \([0-9.]\+\).*$/\1/;p}'`
serverip=100.64.0.1
serverport=8888

cctype=remy
ratname=rats/fig4-rtt/145--155.dna.4 #150-alone.dna.2
remytype= 

#cctype=kernel

for num_senders in 4 #8 12 16
do
	echo "Running for $num_senders sender(s)"
	pids=""

	#sudo tcpdump -i ingress -w tcpdump-senders$num_senders & 
	for (( i=1; i<=$num_senders; i++ ))
	do	
		./sender$remytype if=$ratname cctype=$cctype serverip=$serverip sourceip=$sourceip sourceport=0 serverport=$serverport onduration=5000 offduration=5000 >> range-test-results/$remytype-link$1-delay$2-numsenders$num_senders & pid=$!
		pids+=" $pid"
	done
	sleep 100

	#echo "Warning: Running for only 100s"
	kill -9 $pids
	#sudo killall -9 tcpdump
	killall -9 sender-$remytype
done


# tcptrace -lr tcpdump-senders16 | grep --regex='\(RTT avg\)\|\(throughput\)' > tcptrace-senders16