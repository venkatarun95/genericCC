import argparse
from fractions import Fraction
import math
import os
import pickle
import random
import re

from parsing_scripts import parse_ctcp_output
from parsing_scripts import parse_tcptrace

linkspeed_range = [2, 15] # in MBps
minrtt_range = [20.0, 200.0] # in ms
num_senders_list = [1, 2, 4, 8, 16, 32, 64]
delta_list = [1.0]
rat_source = "input-rats/nash-eval/"
rat_list = []

def discover_rats(directory):
    global rat_list
    if not os.path.isdir(directory):
        print("Could not open '" + directory + "'. Please specify a directory.")
        exit(1)
    try:        
        rat_list = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
            os.path.join(directory, f)) ]
    except:
        print("Error while reading from directory: '" + directory + "'.")

def weighted_means(data):
  """ Returns (average throughput, average rtt) from data"""
  avg_throughput, avg_rtt = 0.0, 0.0
  tot_time, tot_pkts = 0.0, 0.0
  for conn in data:
    try:
      avg_throughput += conn['Throughput'] * conn['TransmitTime']
      avg_rtt += conn['RTT'] * conn['NumPkts']
      tot_time += conn['TransmitTime']
      tot_pkts += conn['NumPkts']
    except:
      print "Error:- "
      print conn

  if tot_time == 0 or tot_pkts == 0:
    return (None, None)
  avg_throughput /= tot_time
  avg_rtt /= tot_pkts

  return (avg_throughput, avg_rtt)

def create_pkl_from_log(base_name, log_type, outfile_name):
	''' Read log files produced by 'sender' program abd create a pickle
	file containing all the data. The format of the data is a list of 
	dictionaries as produced by parse_tcptrace and parse_ctcp_output

	base_name -- name of the file minus suffixes produced by 
		run-senders-parallel.shsuch as '-tcptrace'
	log_type -- Whether log is from ctcp or kernel (ie. tcptrace -lr)
	'''
	assert(os.path.isfile(base_name) or os.path.isfile(base_name+'-tcptrace'))
	assert(log_type in ['ctcp', 'tcptrace'])

	if log_type == 'ctcp':
		data = parse_ctcp_output.parse_file(base_name)
	else:
		data = parse_tcptrace.parse_file(base_name+'-tcptrace', endpt_name='100.64.0.1')
	outfile = open(outfile_name, 'w')
	pickle.dump(data, outfile)
	outfile.close()

def create_trace_file(speed, outfile_name):
	''' Take speed in MBps and converts that to a trace
	file suitable for linkshell. Save it in outfile_name.
	'''
	assert(type(speed) is float)
	assert(type(outfile_name) is str)

	speed = speed/1.500 # convert to packets per ms (a packet is 1500 bytes)
	frac = Fraction(speed).limit_denominator()
	out_file = open(outfile_name, 'w')
	#print "Printing trace for microsecond linkshell"
	for i in range(frac.numerator):
		out_file.write(str(int((i+1)/speed+0.5)) + '\n')
	out_file.close()

def single_mahimahi_run(
	minrtt, 
	linkrate, 
	delta,
	numsenders, 
	ratname, 
	training_linkrate,
	cctype,
	output_directory):
	assert(type(minrtt) is float) # in ms
	assert(type(linkrate) is float) # in MBps
	assert(type(delta) is float and delta > 0.0)
	assert(type(numsenders) is int)
	assert(type(ratname) is str)
	assert(type(training_linkrate) is float) # in MBps
	assert(cctype in ['remy', 'kernel', 'nash', 'markovian'])
	assert(os.path.isdir(output_directory))

	ratname_nice = ratname.split('/')[-1].split('-linkppt')[0]
	if cctype == 'kernel':
		ratname_nice = 'cubic'
	if cctype in ['nash', 'markovian']:
		ratname_nice = cctype + str(delta)
	outfile_name = 'rawout-' + ratname_nice + '-' + str(linkrate) + '-' \
					+ str(minrtt) + '-' + str(numsenders)
	outfile_name = os.path.join(output_directory, outfile_name)

	create_trace_file(linkrate, '/tmp/linkshell-trace')

	try:
		os.remove('tmp-random-range-tests.dat')
	except OSError:
		print "Warning: Unable to remove tmp file"

	queue_length = linkrate * minrtt * 1e6
	print "Queue Length: ", queue_length, " bytes"

	runstr = 'mm-delay ' + str(int(minrtt/2)) \
			+ ' mm-link /tmp/linkshell-trace /tmp/linkshell-trace ' \
			+ 'sudo ./run-senders-parallel.sh ingress 100.64.0.1 8888 ' \
			+ cctype + ' ' + ratname + ' ' + str(delta) + ' ' \
			+ str(numsenders) + ' ' + str(training_linkrate) + ' 0 ' \
			+ 'tmp-random-range-tests.dat' #outfile_name

			# + '--uplink-queue-args="bytes=' + str(queue_length) \
			# + '" --uplink-queue=droptail ' \

	os.system(runstr)

	create_pkl_from_log('tmp-random-range-tests.dat', ('ctcp', 'tcptrace')[cctype == 'kernel'], outfile_name)

def run_tests(output_directory):
	while True:
		linkrate = float(random.randrange(linkspeed_range[0], linkspeed_range[1]))
		minrtt = float(random.randrange(minrtt_range[0], minrtt_range[1]))
		numsenders = num_senders_list[random.randrange(len(num_senders_list))]
		print "Testing for linkrate =", linkrate, "MBps, minrtt =", minrtt,\
			"ms and", numsenders, "senders"

		for delta in delta_list:
			try:
				os.mkdir(os.path.join(output_directory, 'markovian'+str(delta)))
			except OSError:
				pass
			single_mahimahi_run(
				minrtt,
				linkrate,
				delta,
				numsenders,
				'norat',
				1.0,
				'markovian',
				os.path.join(output_directory, 'markovian'+str(delta))
			)
		# for delta in delta_list:
		# 	try:
		# 		os.mkdir(os.path.join(output_directory, 'nash'+str(delta)))
		# 	except OSError:
		# 		pass
		# 	single_mahimahi_run(
		# 		minrtt,
		# 		linkrate,
		# 		delta,
		# 		numsenders,
		# 		'norat',
		# 		1.0,
		# 		'nash',
		# 		os.path.join(output_directory, 'nash'+str(delta))
		# 	)
		# for rat in rat_list:
		# 	try:
		# 		os.mkdir(os.path.join(output_directory, 'remy-'+rat.split('/')[-1]))
		# 	except OSError:
		# 		pass
		# 	single_mahimahi_run(
		# 		minrtt,
		# 		linkrate,
		# 		1.0,
		# 		numsenders,
		# 		rat,
		# 		1.0,
		# 		'remy',
		# 		os.path.join(output_directory, 'remy-'+rat.split('/')[-1])
		# 	)
		# try:
		# 	os.mkdir(os.path.join(output_directory, 'kernel'))
		# except OSError:
		# 	pass
		# single_mahimahi_run(
		# 	minrtt,
		# 	linkrate,
		# 	1.0,
		# 	numsenders,
		# 	'norat',
		# 	1.0,
		# 	'kernel',
		# 	os.path.join(output_directory, 'kernel')
		# )

def weighted_means(data):
  """ Returns (average throughput, average rtt from data)"""
  avg_throughput, avg_rtt = 0.0, 0.0
  tot_time, tot_pkts = 0.0, 0.0
  for conn in data:
    try:
      avg_throughput += conn['Throughput'] * conn['TransmitTime']
      avg_rtt += conn['RTT'] * conn['NumPkts']
      tot_time += conn['TransmitTime']
      tot_pkts += conn['NumPkts']
    except:
      print "Error:- "
      print conn

  if tot_time == 0 or tot_pkts == 0:
    return (None, None)
  avg_throughput /= tot_time
  avg_rtt /= tot_pkts

  return (avg_throughput, avg_rtt)

def analyse_pkl_data(input_directory):
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
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)
	re_mahimahi_nash_name = re.compile(r"""
		rawout-(?P<ratname>nash[0-9.]*)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)
	re_mahimahi_markovian_name = re.compile(r"""
		rawout-(?P<ratname>markovian[0-9.]*)-
		(?P<linkrate>[0-9.]+)-
		(?P<minrtt>[0-9.]+)-
		(?P<numsenders>[0-9]+)$
	""", re.VERBOSE)

	dir_names = [ os.path.join(input_directory, f) for f in os.listdir(input_directory) if os.path.isdir(
          os.path.join(input_directory, f)) ]
	for directory in dir_names:
		file_names = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
          os.path.join(directory, f)) ]
		directory = directory.split('/')[-1]

		master_data_list = []

		for filename in file_names:
			infile = open(filename, 'r')
			filename = filename.split('/')[-1]
			if directory.find('kernel') != -1:
				match = re_mahimahi_kernel_name.match(filename)
			elif directory.find('remy') != -1:
				match = re_mahimahi_name.match(filename)
			elif directory.find('nash') != -1:
				match = re_mahimahi_nash_name.match(filename)
			elif directory.find('markovian') != -1:
				match = re_mahimahi_markovian_name.match(filename)
			else:
				print "Cannot understand directory name: ", directory
				break

			linkrate, minrtt, numsenders = float(match.group('linkrate')), \
				float(match.group('minrtt')), int(match.group('numsenders'))
			data = pickle.load(infile)
			infile.close()
			for x in data:
				x['Throughput'] /= 1e6 # convert to MBps
				x['Throughput'] /= linkrate / ((numsenders+1) / 2) # normalize w.r.t optimum
				# x['Throughput'] /= (numsenders/2.0) / ((numsenders/2.0) + 1.0)
				# x['RTT'] -= minrtt # find queueing delay
				x['RTT'] /= minrtt
				# print x['RTT'], minrtt
				x['RTT'] *= linkrate * 1e3 / 1500.0 #normalize and convert to pkts in queue
			master_data_list.extend(data)

		res = weighted_means(master_data_list)
		print directory, "\t", res#, res[0] / math.sqrt(res[1]), res[0] / res[1]


if __name__ == "__main__":
	argparser = argparse.ArgumentParser()
	argparser.add_argument('--output_directory', type=str, help='Directory where the pkl files are to be stored')
	argparser.add_argument('--input_directory', type=str, help='Directory where pkl files are stored for analysis')
	cmd_line_args = argparser.parse_args()

	if cmd_line_args.input_directory == None and cmd_line_args.output_directory != None:
		discover_rats(rat_source)
		run_tests(cmd_line_args.output_directory)
	elif cmd_line_args.input_directory != None and cmd_line_args.output_directory == None:
		analyse_pkl_data(cmd_line_args.input_directory)
	else:
		print "Please specify exactly one of input directory and output directory"
