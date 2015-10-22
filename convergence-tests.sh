#!/bin/bash

trash convergence-test-tcpdump
sudo tcpdump -i ingress -w convergence-test-tcpdump & tcpdump_pid=$!

shortest_duration=5000
delta=0.1
cctype=remy

./sender \
	serverip=100.64.0.1 \
	sourceip=100.64.0.4 \
	if=rats/fig2-linkspeed/bigbertha-1000x.dna.5 \
	cctype=$cctype \
	delta=$delta \
	max_delay=200  \
	offduration=0 \
	onduration=$((shortest_duration*7)) \
	traffic_params=deterministic,num_cycles=1 &

./sender \
	serverip=100.64.0.1 \
	sourceip=100.64.0.4 \
	if=rats/fig2-linkspeed/bigbertha-1000x.dna.5 \
	cctype=$cctype \
	delta=$delta max_delay=200  \
	offduration=$((shortest_duration*1)) \
	onduration=$((shortest_duration*5)) \
	traffic_params=deterministic,num_cycles=1 &

./sender \
	serverip=100.64.0.1 \
	sourceip=100.64.0.4 \
	if=rats/fig2-linkspeed/bigbertha-1000x.dna.5 \
	cctype=$cctype \
	delta=$delta max_delay=200  \
	offduration=$((shortest_duration*2)) \
	onduration=$((shortest_duration*3)) \
	traffic_params=deterministic,num_cycles=1 &

./sender \
	serverip=100.64.0.1 \
	sourceip=100.64.0.4 \
	if=rats/fig2-linkspeed/bigbertha-1000x.dna.5 \
	cctype=$cctype \
	delta=$delta max_delay=200  \
	offduration=$((shortest_duration*3)) \
	onduration=$((shortest_duration*1)) \
	traffic_params=deterministic,num_cycles=1 &

sleep $((shortest_duration*7/1000))
	sudo kill $tcpdump_pid

echo "Running tcpdump"
trash convergence-test-tcpdump-ascii
sudo tcpdump -r convergence-test-tcpdump > convergence-test-tcpdump-ascii

# Find the port numbers (and the ip addresses)
awk '{print $3}' convergence-test-tcpdump-ascii | sort -u | grep 100\.64\.0\.4\..* > /tmp/unique_ports

# Separate the sent packets into different files, segregated by sender port
gnuplot_script="plot " 
i=1
echo "" > /tmp/r_script
while read p; do
	grep $p\ \> convergence-test-tcpdump-ascii | \
		awk '{print $1}' | \
		awk -F ':' 'BEGIN{prev=0}{print $2*60+$3, 1/($2*60+$3-prev);prev=$2*60+$3}' > /tmp/Sender_$p
  	echo "Sender_$p"
  	gnuplot_script="$gnuplot_script \"/tmp/Sender_$p\" using 1:2, " 
  	
  	#i=`echo $p | awk -F '.'{print $5}`
  	echo "v_$i <- read.csv(file='/tmp/Sender_$p', head=FALSE, sep=' ')" >> /tmp/r_script
  	echo "h_$i <- hist(v_$i\$V1, breaks=(max(v_$i\$V1) - min(v_$i\$V1))*3)" >> /tmp/r_script # 10 breaks per 1ms
  	i=$((i+1))
done </tmp/unique_ports

# echo "plot(h_1, col=rgb(1,0,0,1/4), xlim=c(min(min(v_3\$V1), min(v_2\$V1), min(v_1\$V1)), max(max(v_3\$V1), max(v_2\$V1), max(v_1\$V1))))" >> /tmp/r_script
echo "plot(h_1, col=rgb(1,0,0,1/4), xlim=c(min(min(v_4\$V1), min(v_3\$V1), min(v_2\$V1), min(v_1\$V1)), max(max(v_4\$V1), max(v_3\$V1), max(v_2\$V1), max(v_1\$V1))), ylim=c(0, max(max(h_1\$counts), max(h_2\$counts), max(h_3\$counts), max(h_4\$counts))))" >> /tmp/r_script 
echo "plot(h_2, col=rgb(0,1,0,1/4), add=T)" >> /tmp/r_script
echo "plot(h_3, col=rgb(0,0,1,1/4), add=T)" >> /tmp/r_script
echo "plot(h_4, col=rgb(1,1,0,1/4), add=T)" >> /tmp/r_script

echo $gnuplot_script | head -c -2 > /tmp/gnuplot_script
echo $gnuplot_script
cat /tmp/r_script
# gnuplot /tmp/gnuplot_script 