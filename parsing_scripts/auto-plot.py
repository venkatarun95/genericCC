import argparse
import numpy as np
import os
import random
import re

import plot_ellipse

def read_remy_datafile(filename, pts):
    re_num = re.compile(".*\s([0-9.e+]+)\.*")

    infile = open(filename, 'r')
    line = infile.readline()
    while line != "":
        if line.find('Data Successfully Transmitted') != -1:
            try:
                line = infile.readline()
                throughput = float( re_num.match( line ).group( 1 ) )*8
                line = infile.readline()
                rtt = float( re_num.match( line ).group( 1 ) ) * 1000
            except:
                # invalid observatio, probably bedause the time was too short for even one packet to get through
                continue
            if rtt == 0 or throughput == 0:
                continue
            if throughput < 1e4:
                print throughput
                #continue
            pts.append( [rtt, throughput] )
        else:
            line = infile.readline()

def read_tcp_datafile(filename, pts):
    re_rtt = re.compile('\s*RTT avg:\s*([\d.]+)[\D\s]+([\d.]+).*')
    re_throughput = re.compile('\s*throughput:\s*([\d.]+)[\D\s]+([\d.]+).*')

    cur_throughput = None # keep track of throughput so that we can append to pts when we reach rtt

    for line in open(filename, 'r').readlines():
        if line.find('TCP connection') != -1:
            cur_throughput = None
        if line.find('RTT avg:') != -1:
            try:
                vals = re_rtt.match(line).groups()
            except AttributeError as e:
                #print "Attribute error at '" + line + "'"
                continue
            except ValueError:
                print "Value error at '" + line + "'"
                continue

            if cur_throughput == None:
                print "Warning: Throughput is None though rtt was found"
                continue

            cur_rtt = max( float(vals[0]), float(vals[1]) )
            if cur_rtt == 0 or cur_throughput == 0:
                continue
            pts.append( [cur_rtt, cur_throughput] )
        elif line.find('throughput:') != -1:
            try:
                vals = re_throughput.match(line).groups()
            except AttributeError:
                #print "Attribute error at '" + line + "'"
                continue
            except ValueError:
                print "Value error at '" + line + "'"
                continue

            cur_throughput = max( int(vals[0]), int(vals[1]) )
            cur_throughput *= 8 # convert to bps

def collect_data_from_directory(directory):
    re_kernel_name = re.compile('kernel.*-([0-9]+)-tcptrace')
    re_remy_name = re.compile('remy-(.*)-([0-9]+)')
    re_nash_name = re.compile('(nash[^-]*)-.*-([0-9]+)')
    file_names = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
            os.path.join(directory, f)) ]

    data_bins = {} # key: (num_senders, cc_name) value: Nx2 list of values as [ [tp1, rtt1], [tp2, rtt2], ... ]
    num_senders_list = {} # key: int, value: None
    rat_names = {'kernel': None} # key: name of rat/'kernel'/'nash', value: None

    for full_fname in file_names:
        fname = full_fname.split('/')[-1]
        if fname.startswith('kernel') and fname.endswith('tcptrace'):
            num_senders = int(re_kernel_name.match(fname).group(1))
            if num_senders not in num_senders_list:
                num_senders_list[num_senders] = None
            
            if (num_senders, 'kernel') not in data_bins:
                data_bins[(num_senders, 'kernel')] = []

            read_tcp_datafile(full_fname, data_bins[(num_senders, 'kernel')])

        elif fname.startswith('remy'):
            tmp = re_remy_name.match(fname).groups()
            remy_name, num_senders = tmp[0], int(tmp[1])
#            assert((num_senders, remy_name) not in data_bins)
            if num_senders not in num_senders_list:
                num_senders_list[num_senders] = None
            if fname not in rat_names:
                rat_names[remy_name] = None

            if (num_senders, remy_name) not in data_bins:
                data_bins[(num_senders, remy_name)] = []

            read_remy_datafile(full_fname, data_bins[(num_senders, remy_name)])

        elif fname.startswith('nash'):
            tmp = re_nash_name.match(fname).groups()
            nash_name, num_senders = tmp[0], int(tmp[1])
            if nash_name == 'nash':
                nash_name = 'nash1'
                print "Renaming 'nash'"
#            assert((num_senders, remy_name) not in data_bins)
            if num_senders not in num_senders_list:
                num_senders_list[num_senders] = None
            if nash_name not in rat_names:
                rat_names[nash_name] = None
            
            if (num_senders, nash_name) not in data_bins:
                data_bins[(num_senders, nash_name)] = []

            read_remy_datafile(full_fname, data_bins[(num_senders, nash_name)])
    #for key in data_bins:
    #    print key, len(data_bins[key])
    #print [x for x in rat_names]
    plot_graphs(data_bins, num_senders_list, rat_names)

def plot_graphs(data_bins, num_senders_list, rat_names):
    color_list = ['r', 'g', 'b', 'c', 'm', 'y', 'k']
    color_ctr = 0
    colors = {} #key: ratname, value: color string
    for x in rat_names:
        colors[x] = color_list[color_ctr]
        print color_list[color_ctr], x
        color_ctr += 1

    for num_senders in  num_senders_list:
        print num_senders, "Senders"
        for datapt in data_bins:
            if datapt[0] != num_senders:
                continue
            if datapt[1] == '145--155-linkppt2667.dna.4-0':# or datapt[1] == 'nash0.1' or datapt[1] == 'nash1':
                print "Ignoring ", datapt
                continue
            #plot_ellipse.plot_point_cov(np.asarray(data_bins[datapt]), nstd=1, color=colors[datapt[1]], alpha=0.5)
            rnd = [random.random() < 0.3 for x in data_bins[datapt]]
            plot_ellipse.plt.plot([data_bins[datapt][i][0] for i in range(len(data_bins[datapt])) if rnd[i]], [data_bins[datapt][i][1] for i in range(len(data_bins[datapt])) if rnd[i]], colors[datapt[1]]+'o', alpha=0.5, label=datapt[1])

        plot_ellipse.plt.gca().invert_xaxis()
        plot_ellipse.plt.xscale('log')
        plot_ellipse.plt.xlabel('Queueing Delay (ms)')
        plot_ellipse.plt.ylabel('Throughput (bps)')
        plot_ellipse.plt.title(str(num_senders) + ' Sender(s)')
        plot_ellipse.plt.legend()
        plot_ellipse.plt.show()


if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Plots data from the raw files output by automatic-tests.py.')
    argparser.add_argument('--input_directory', metavar='I', type=str, help='Directory where the raw output files are.')
    argparser.add_argument('--output_directory', metavar='O', type=str, help='Directory where the graphs should be written to.')
    cmd_line_args = argparser.parse_args()

    collect_data_from_directory(cmd_line_args.input_directory)