import argparse
from fractions import Fraction
import math
import matplotlib.pyplot as plt
import numpy as np
import os
import re
import time

import parse_ctcp_output
import analyse_data

# Format:-
# 	key: type eg. 10x, 1000x, cubic, cubicsfqCodel etc.
#	value: (type, value, filenames) where
#		type: one of 'link' or 'delay', depending on which is varied
#		value: value of the link speed (Mbps) or delay (ms)
ns_files = {}
def preprocess_ns_direcotry(directory):
	''' Parse all filenames in directory containing ns2 simulation
		results and store it so that the appropriate filenames can
		be rapidly retrieved
	'''
	pass

def plot_data(mahimahi_directory, remysim_directory, vary_link, delta=1):
	''' Plot the utility function w.r.t link rate or minrtt

		mahimahi_directory -- directory where raw results 
			from conduct_mahimahi_expt are stored
		remysim_directory -- directory where results from the remy 
			simulator are stored
		vary_link -- if True, it is assumed that the link rate is to
			be varied. Else minrtt is varied
		delta -- adjustable factor in computing utility
	'''
	assert(type(mahimahi_directory) is str)
	assert(type(remysim_directory) is str)
	assert(type(vary_link) is bool)

	re_mahimahi_name = re.compile(r"""
		rawout-(?P<ratname>.*)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)
	re_remysim_name = re.compile(r"""
		rawout-remysim-(?P<ratname>.*)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)

	re_remysim_data = re.compile(r"sender: \[tp=(?P<throughput>[0-9.]+), del=(?P<delay>[0-9.]+)\]")

	file_names = [ os.path.join(mahimahi_directory, f) for f in os.listdir(mahimahi_directory) 
		if os.path.isfile(os.path.join(mahimahi_directory, f)) ]

	# if link speed is varied, fixed value the rtt, else it is the link speed
	fixed_value = -1
	# format: key - ratname, value - [(linkrate or rtt, utility), ...]
	values = {}

	tmp_values = []
	for filename in file_names:
		if filename.find('remysim') != -1:
			match = re_remysim_name.match(filename.split('/')[-1])
			remysim = True
		else:
			match = re_mahimahi_name.match(filename.split('/')[-1])
			remysim = False
		if match is None:
			continue

		linkrate, minrtt = match.group('linkrate'), match.group('minrtt')
		ratname = match.group('ratname') + ('', '-remysim')[remysim]
		
		if filename.find('remysim') != -1 or float(linkrate) < 1.25:
			continue

		if ratname not in values:
			values[ratname] = []
		
		if remysim:
			infile = open(filename,	'r')
			throughput, delay, numsenders = 0, 0, 0
			for line in infile.readlines():
				line_match = re_remysim_data.match(line)
				if line_match == None:
					continue
				throughput += float(line_match.group('throughput'))
				delay += float(line_match.group('delay'))
				numsenders += 1
			if numsenders < 2:
				print "Skipping ", filename
				continue
			print filename
			assert( numsenders == 2 )
			throughput, delay = throughput/numsenders, delay/numsenders
			throughput /= (numsenders + 1)/2.0 # divide by E[ #senders ]
			throughput /= float(linkrate) # normalize

		else:
			data = parse_ctcp_output.parse_file(filename)
			throughput, delay = analyse_data.weighted_means(data)
			if throughput == None or delay == None:
				print "Warning: No data present in ", filename
				continue
			throughput /= 1e6 * float(linkrate) # convert to MBps and normalize
			throughput *= 1500.0 / 1468.0 # compensate for differences in data sizes

		tmp_values.append( (linkrate, throughput, delay, remysim) )

		utility = math.log(throughput, 10) - delta*math.log(delay, 10)

		if vary_link:
			fixed_value = minrtt
			values[ratname].append( (linkrate, utility) )
		else:
			fixed_value = linkrate
			values[ratname].append( (minrtt, utility) )

	tmp_values.sort(cmp=lambda x,y: ((1, -1)[x[0] > y[0]], 0)[x[0] == y[0]])
	for x in tmp_values: print x

	colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k'] 
	color_ctr = 0
	print "For " + ('rtt', 'link rate')[vary_link] + " = " + str(fixed_value)
	for rat in values:
		print rat, '\t', colors[color_ctr]
		x, y = zip(*values[rat])
		plt.plot(x, y, colors[color_ctr]+'o', alpha=0.8, label=rat)
		color_ctr += 1
	plt.xlabel('Link rate (MBps)')
	plt.ylabel('Utility')
	plt.xscale('log')
	#plt.legend()
	plt.show()

if __name__ == '__main__':
	argparser = argparse.ArgumentParser(description='Plot raw results obtained from automatic-tests_vary-mahimahi-configs')
	argparser.add_argument('--input_directory', type=str, default="", help="Directory where results are stored")
	argparser.add_argument('--vary_link', type=bool, default="", help="If specified, assumes link rate is varied. Else min rtt is assumed to be varied")
	argparser.add_argument('--ns_results', type=str, help="Directory where results from ns simulations are stored")

	cmd_line_args = argparser.parse_args()

	plot_data(cmd_line_args.input_directory, "", cmd_line_args.vary_link)