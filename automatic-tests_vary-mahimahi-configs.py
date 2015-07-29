import argparse
from fractions import Fraction
import math
import os
import time

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
	assert(cctype in ['remy', 'kernel', 'nash'])
	assert(os.path.isdir(output_directory))

	ratname_nice = ratname.split('/')[-1].split('-linkppt')[0]
	if cctype == 'kernel':
		ratname_nice = 'cubic'
	if cctype == 'nash':
		ratname_nice = 'nash' + str(delta)
	out_filename = 'rawout-' + ratname_nice + '-' + str(linkrate) + '-' \
					+ str(minrtt) + '-' + str(numsenders)
	out_filename = os.path.join(output_directory, out_filename)

	create_trace_file(linkrate, '/tmp/linkshell-trace')

	runstr = 'mm-delay ' + str(int(minrtt/2)) \
			+ ' mm-link /tmp/linkshell-trace /tmp/linkshell-trace ' \
			+ 'sudo ./run-senders-parallel.sh ingress 100.64.0.1 8888 ' \
			+ cctype + ' ' + ratname + ' ' + str(delta) + ' ' \
			+ str(numsenders) + ' ' + str(training_linkrate) + ' 0 ' \
			+ out_filename

	os.system(runstr)

def single_remysim_run(
	minrtt,
	linkrate,
	numsenders,
	ratname,
	output_directory):
	assert(type(minrtt) is float) # in ms
	assert(type(linkrate) is float) # in MBps
	assert(type(numsenders) is int)
	assert(type(ratname) is str)
	assert(os.path.isdir(output_directory) or cctype != 'remy')

	ratname_nice = ratname.split('/')[-1].split('-linkppt')[0]
	out_filename = 'rawout-remysim-' + ratname_nice + '-' + str(linkrate) + '-' \
					+ str(minrtt) + '-' + str(numsenders)
	out_filename = os.path.join(output_directory, out_filename)

	linkrate = linkrate*1e3/1500.0 # convert from MBps to pkts per ms (ppt)

	runstr = '~/Documents/Projects/Remy/remy/src/rat-runner' \
			+ ' link=' + str(linkrate) + ' rtt=' + str(minrtt) \
			+ ' if=' + ratname + ' nsrc=' + str(numsenders) \
			+ ' on=5000 off=5000 >> ' + out_filename + ' 2>&1'

	os.system( runstr )

def conduct_expt(args):
	''' Depending on expt_type, either runs genericCC on mahimahi (if 
		args.expt_type='mahimahi') or runs a simulation on the remy 
		simulator (if args.expt_type='remysim'). args can be arguments from
		the command line. Refer function body for what should be there.
	'''
	#resolution = math.pow((args.upper - args.lower), 1 / float(args.num_points))
	resolution = (args.upper - args.lower) / float(args.num_points-1)
	#resolution = 1.0 / args.num_points
	mult = math.sqrt(float(args.upper - args.lower)) / args.num_points
	for i in range(0, args.num_points):
		if args.vary_link:
			linkrate = args.lower + (mult*i)**2
			rtt = args.minrtt
		else:
			linkrate = args.linkrate
			rtt = args.lower + resolution*i
		print "Running for " + str(linkrate) + " MBps " + str(rtt) + " ms"
		if args.expt_type == 'mahimahi':
			single_mahimahi_run(
				rtt, 
				linkrate, 
				args.delta,
				args.num_senders, 
				args.ratname, 
				1.0,	# currently not supporting link rate normalization
				args.cctype,
				args.output_directory
			)
			time.sleep(5)
		elif args.expt_type == 'remysim':
			single_remysim_run(
				rtt, 
				linkrate, 
				args.num_senders, 
				args.ratname, 
				args.output_directory
			)
		else:
			assert(False)

if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Run the given rat over various throughput-delay configurations.')
    argparser.add_argument('--ratname', type=str, help='Directory where the raw output files are')
    argparser.add_argument('--output_directory', type=str, help='Output files go here')
    argparser.add_argument('--vary_link', type=bool, help="If true, varies link rate, else varies rtt")
    argparser.add_argument('--num_points', type=int, default=10, help="Number of datapoints at which to evaluate")
    argparser.add_argument('--lower', type=float, help="Lower limit. In MBps or ms")
    argparser.add_argument('--upper', type=float, help="Upper limit. In MBps or ms")
    argparser.add_argument('--expt_type', type=str, help="One of 'remysim' or 'mahimahi'")
    argparser.add_argument('--minrtt', type=float, default=150.0, help="Minimum possible rtt in link in ms")
    argparser.add_argument('--linkrate', type=float, default=4.0, help="Link rate in MBps")
    argparser.add_argument('--delta', type=float, default=1.0, help="Delta for NashCC")
    argparser.add_argument('--num_senders', type=int, default=2, help="Number of senders")
    argparser.add_argument('--cctype', type=str, default='remy', help="Should be one of 'remy' and 'kernel'")
    cmd_line_args = argparser.parse_args()

    run_repeated = cmd_line_args.expt_type == 'mahimahi'
    conduct_expt(cmd_line_args)
    #while run_repeated:
    #conduct_expt(cmd_line_args)