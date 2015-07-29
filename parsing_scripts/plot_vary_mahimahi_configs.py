import argparse
from fractions import Fraction
import math
import matplotlib.pyplot as plt
import numpy as np
import os
import re
import time

import parse_ctcp_output
import parse_tcptrace
import analyse_data

# Format:-
# 	key: cc_type eg. 10x, 1000x, cubic, cubicsfqCodel etc.
#	value: dictionary: key - (vary_type, value), value - [filenames]
#		vary_type: one of 'link' or 'delay', depending on which is varied.
#		value: value of the link speed (Mbps) or delay (ms).
#		filenames: list of filenames satisfying the criteria. They will
#			be different runs of the same configuration.
ns_files = {}
def preprocess_ns_directory(directory):
	""" Parse all filenames in directory containing ns2 simulation
		results and store it so that the appropriate filenames can
		be rapidly retrieved
	"""
	global ns_files
	file_names = [ os.path.join(directory, f) for f in os.listdir(directory) 
		if os.path.isfile(os.path.join(directory, f)) ]

	re_ns_filename = re.compile(r"""
		(?P<cc_type>\w+)-
		(?P<vary_type>link|delay)
		(?P<value>[\d.]+)
		run\d+.err
		""", re.VERBOSE)

	for filename in file_names:
		match = re_ns_filename.match(filename.split('/')[-1])
		if match is None:
			continue
		cc_type = match.group('cc_type')
		file_key = (match.group('vary_type'), float(match.group('value')))
		if cc_type not in ns_files:
			ns_files[cc_type] = {}
		if file_key not in ns_files[cc_type]:
			ns_files[cc_type][file_key] = []
		ns_files[cc_type][file_key].append( filename )

def get_nearest_ns_throughput_delay(cc_type, vary_type, value):
	""" Returns (throughput, delay) from ns2 simulation results given
		cctype (which can be a ratname or 'cubic' or 'cubicsfqCoDel'), 
		vary_type (one of 'link' and 'delay') and value (link rate or
		delay for which closest match has to be found)
	"""
	assert(type(cc_type) is str)
	assert(vary_type in ['link', 'delay'])
	assert(type(value) is float)
	global ns_files

	# canonicalize cc_type
	for x in ['2x', '10x', '100x', '1000x']:
		if x in cc_type:
			cc_type = x
			break
	if cc_type not in ['2x', '10x', '100x', '1000x']:
		assert(cc_type in ['cubic', 'cubicsfqCoDel'])

	# find closest match
	closest_value, closest_filenames = -1e9, []
	for x in ns_files:
		if x != cc_type:
			continue
		for y in ns_files[x]:
			if y[0] != vary_type:
				continue
			if abs(y[1] - value) < abs(closest_value - value):
				closest_value = y[1]
				closest_filenames = ns_files[x][y]
	assert(closest_value > 0)
	
	# parse the files
	re_ns_result_line = re.compile(r"""
		.*tp=(?P<throughput>[\d.]+) #mbps
		.*del=(?P<delay>[\d.]+) #ms
		.*on=(?P<duration>[\d.]+) #secs
		.*
		""", re.VERBOSE)
	delay = 0
	tot_time, tot_bytes = 0, 0 # for normalizing throughput and delay resp.
	for filename in closest_filenames:
		infile = open(filename, 'r')
		for line in infile.readlines():
			match = re_ns_result_line.match(line)
			if match is None:
				continue
			duration = float(match.group('duration'))
			throughput = float(match.group('throughput'))
			delay += float(match.group('delay')) * throughput * duration
			tot_time += duration
			tot_bytes += throughput * duration # exact units don't matter
	if tot_time == 0  or tot_bytes == 0:
		print "Warning: Nothing found for ", (cc_type, vary_type, value)
		return (1, 1)
	throughput = tot_bytes / tot_time
	delay /= tot_bytes

	return (throughput, delay)

def plot_data(mahimahi_directory, remysim_directory, vary_link, delta=1):
	""" Plot the utility function w.r.t link rate or minrtt

		mahimahi_directory -- directory where raw results 
			from conduct_mahimahi_expt are stored
		remysim_directory -- directory where results from the remy 
			simulator are stored
		vary_link -- if True, it is assumed that the link rate is to
			be varied. Else minrtt is varied
		delta -- adjustable factor in computing utility
	"""
	assert(type(mahimahi_directory) is str)
	assert(type(remysim_directory) is str)
	assert(type(vary_link) is bool)

	re_mahimahi_name = re.compile(r"""
		rawout-(?P<ratname>.*)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)
	re_mahimahi_kernel_name = re.compile(r"""
		rawout-(?P<ratname>cubic)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)-tcptrace$
	""", re.VERBOSE)
	re_mahimahi_nash_name = re.compile(r"""
		rawout-(?P<ratname>nash[0-9.]*)-
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
		elif filename.find('cubic') != -1:
			match = re_mahimahi_kernel_name.match(filename.split('/')[-1])
			remysim = False
		elif filename.find('nash') != -1:
			match = re_mahimahi_nash_name.match(filename.split('/')[-1])
			remysim = False
		else:
			match = re_mahimahi_name.match(filename.split('/')[-1])
			remysim = False
		if match is None:
			continue

		linkrate, minrtt = match.group('linkrate'), match.group('minrtt')
		ratname = match.group('ratname') + ('', '-remysim')[remysim] + ' ' + match.group('numsenders') + 'senders'
		print ratname, linkrate, minrtt

		if filename.find('remysim') != -1 or filename.find('us-') != -1:
			print "Ignoring ", filename
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
			assert( numsenders == 2 )
			throughput, delay = throughput/numsenders, delay/numsenders
			throughput /= (numsenders + 1)/2.0 # divide by E[ #senders ]
			throughput /= float(linkrate) # normalize
			delay -= float(minrtt)

		else:
			if filename.split('/')[-1].find('cubic') != -1:
				data = parse_tcptrace.parse_file(filename, endpt_name="100.64.0.1", dst_endpt_name="100.64.0.1")
			else:
				data = parse_ctcp_output.parse_file(filename)
			throughput, delay = analyse_data.weighted_means(data)
			if throughput == None or delay == None:
				print "Warning: No data present in ", filename
				continue
			throughput /= 1e6 * float(linkrate) # convert to MBps and normalize
			throughput *= 1500.0 / 1468.0 # compensate for differences in data sizes
			delay -= float(minrtt)
			#delay /= float(minrtt)

		utility = math.log(throughput, 10) - delta*math.log(delay, 10)

		if vary_link:
			fixed_value = minrtt
			value = float(linkrate)*8
		else:
			fixed_value = linkrate*8
			value = float(minrtt)

		values[ratname].append( (value, utility) )
		
		# ns_throughput, ns_delay = get_nearest_ns_throughput_delay(
		# 	ratname, 
		# 	('delay', 'link')[vary_link],
		# 	value
		# )
		# ns_throughput /= float(linkrate) * 8 
		# ns_utility = math.log(ns_throughput, 10) - delta*math.log(ns_delay, 10)
		
		# if ratname+'-ns2' not in values:
		# 	values[ratname+'-ns2'] = []
		# values[ratname+'-ns2'].append( (value, ns_utility) )

		tmp_values.append( (value, throughput, delay, ('mahimahi', 'remysim')[remysim]) )
		# tmp_values.append( (value, ns_throughput, ns_delay, 'ns') )

	tmp_values.sort(cmp=lambda x,y: ((1, -1)[x[0] > y[0]], 0)[x[0] == y[0]])
	for x in tmp_values: print x

	colors = ['r', 'g', 'b', 'c', 'm', 'y', 'k', '0.75', '0.5', '0.25', '#663311', '#113366'] 
	color_ctr = 0
	print "For " + ('rtt', 'link rate')[vary_link] + " = " + str(fixed_value)
	
	for rat in values:
		if rat.find('ns2') != -1: 
			continue
		print rat, '\t', colors[color_ctr]
		values[rat].sort(cmp=lambda x,y: ((-1, 1)[x[0] > y[0]], 0)[x[0]==y[0]])
		try:
			x, y = zip(*values[rat])
		except ValueError:
			print "Warning: No data for rat '" + rat + "'"
			continue
		plt.plot(x, y, colors[color_ctr], alpha=0.8, label=rat)
		color_ctr += 1
	if vary_link:
		plt.xlabel('Link rate (Mbps)')
		plt.xscale('log')
	else:
		plt.xlabel('Min. RTT (ms)')
	plt.ylabel('Utility (mahimahi)')
	plt.legend(loc='lower center', bbox_to_anchor=(0.5, 0))
	plt.show()

	color_ctr = 0
	for rat in values:
		if rat.find('ns2') == -1: 
			continue
		print rat, '\t', colors[color_ctr]
		values[rat].sort(cmp=lambda x,y: ((-1, 1)[x[0] > y[0]], 0)[x[0]==y[0]])
		try:
			x, y = zip(*values[rat])
		except ValueError:
			print "Warning: No data for rat '" + rat + "'"
			continue
		plt.plot(x, y, colors[color_ctr], alpha=0.8, label=rat)
		color_ctr += 1
	if vary_link:
		plt.xlabel('Link rate (Mbps)')
		plt.xscale('log')
	else:
		plt.xlabel('Min. RTT (ms)')
	plt.ylabel('Utility (ns2)')
	plt.legend(loc='lower center', bbox_to_anchor=(0.5, 0))
	plt.show()

if __name__ == '__main__':
	argparser = argparse.ArgumentParser(description='Plot raw results obtained from automatic-tests_vary-mahimahi-configs')
	argparser.add_argument('--input_directory', type=str, default="", help="Directory where results are stored")
	argparser.add_argument('--vary_link', type=bool, default="", help="If specified, assumes link rate is varied. Else min rtt is assumed to be varied")
	argparser.add_argument('--ns_results', type=str, help="Directory where results from ns simulations are stored")

	cmd_line_args = argparser.parse_args()

	print "Preprocessing ns results directory"
	preprocess_ns_directory(cmd_line_args.ns_results)
	plot_data(cmd_line_args.input_directory, "", cmd_line_args.vary_link)