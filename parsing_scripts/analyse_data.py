import argparse
import numpy as np
import os
import random
import re

import parse_ctcp_output
import parse_tcptrace
import plot_ellipse

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

def analyse_remy_with_kernel(directory, endpt_name):
  file_names = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
            os.path.join(directory, f)) ]

  def filename_compare(x,y):
    x, y = ''.join(x.split('-')[1:]), ''.join(y.split('-')[1:])
    if x < y:
      return -1
    elif x > y:
      return 1
    else:
      return 0
  file_names.sort(cmp=filename_compare)

  for filename in file_names:
    filename_short = filename.split('/')[-1]
    dat = None
    if filename_short.endswith('tcptrace'):
      dat = weighted_means(parse_tcptrace.parse_file(filename, print_csv=False, endpt_name=endpt_name))
    elif filename_short.startswith('remy') and not filename_short.endswith('prober'):
      dat = weighted_means(parse_ctcp_output.parse_file(filename, print_csv=False))      
    if dat != None:
      if dat[0] == None or dat[0] == None:
        print filename_short, "\t\t\t\t\t", dat
      else:
        print filename_short, '\t\t\t\t\t', (dat[0]/1e6, dat[1])

def collect_data_from_directory(directory, endpt_name, output_directory="/tmp/"):
  re_kernel_name = re.compile('kernel.*-([0-9]+)-tcptrace')
  re_remy_name = re.compile('remy-(.*)-([0-9]+)')
  file_names = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
          os.path.join(directory, f)) ]

  data_bins = {} # key: (num_senders, cc_name) value: tuple of 2 lists ([[tp1, rtt1], [tp2, rtt2], ...], [[tp_wt1, rtt_wt1], [...], ...]) of values as [ ([tp1, rtt1],  [wt_tpt, wt_rtt]), ([tp2, rtt2],  [wt_tpt, wt_rtt]), ... ]
  num_senders_list = {} # key: int, value: None
  rat_names = {'kernel': None} # key: name of rat/'kernel', value: None

  for full_fname in file_names:
      fname = full_fname.split('/')[-1]
      if fname.startswith('kernel') and fname.endswith('tcptrace'):
          num_senders = int(re_kernel_name.match(fname).group(1))
          if num_senders not in num_senders_list:
              num_senders_list[num_senders] = None
          print full_fname
          
          #assert( (num_senders, 'kernel') not in data_bins )
          temp = parse_tcptrace.parse_file(full_fname, print_csv=False, endpt_name=endpt_name, dst_endpt_name=endpt_name)
          data_bins[(num_senders, 'kernel')] = ( [[x['RTT'], x['Throughput']] for x in temp],
                                                [[x['TransmitTime'], x['NumPkts']] for x in temp] )

      elif fname.startswith('remy') and not fname.endswith('prober') and not fname.endswith('tcptrace'):
          tmp = re_remy_name.match(fname).groups()
          remy_name, num_senders = tmp[0], int(tmp[1])
          if num_senders not in num_senders_list:
              num_senders_list[num_senders] = None
          if fname not in rat_names:
              rat_names[remy_name] = None

          assert( (num_senders, remy_name) not in data_bins )
          temp = parse_ctcp_output.parse_file(full_fname, print_csv=False)
          data_bins[(num_senders, remy_name)] = ( [[x['RTT'], x['Throughput']] for x in temp],
                                                [[x['TransmitTime'], x['NumPkts']] for x in temp] )
  plot_graphs(data_bins, num_senders_list, rat_names, output_directory)

def plot_graphs(data_bins, num_senders_list, rat_names, output_directory):
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
      if len(data_bins[datapt][0]) == 0 or len(data_bins[datapt][1]) == 0:
        print "Warning: Empty data list for ", datapt
        continue
      plot_ellipse.plot_point_cov(np.asarray(data_bins[datapt][0]), weights=np.asarray(data_bins[datapt][1]), nstd=1, color=colors[datapt[1]], alpha=0.5)
      rnd = [random.random() < 1 for x in data_bins[datapt][0]]
      plot_ellipse.plt.plot([data_bins[datapt][0][i][0] for i in range(len(data_bins[datapt][0])) if rnd[i]], [data_bins[datapt][0][i][1] for i in range(len(data_bins[datapt][0])) if rnd[i]], colors[datapt[1]]+'o', alpha=0.1)
      #plot_ellipse.plt.plot([x[1] for x in data_bins[datapt][0]], [x[0] for x in data_bins[datapt][0]], colors[datapt[1]]+'o', alpha=0.5)

    plot_ellipse.plt.gca().invert_xaxis()
    #plot_ellipse.plt.yscale('log')
    plot_ellipse.plt.axes().autoscale_view()
    plot_ellipse.plt.xlabel('RTT (ms)')
    plot_ellipse.plt.ylabel('Throughput (Bps)')
    plot_ellipse.plt.title('GPO -> MIT: ' + str(num_senders) + ' senders')
    #plot_ellipse.plt.show()
    plot_ellipse.plt.savefig(os.path.join(output_directory, str(num_senders)+"_senders"))
    plot_ellipse.plt.close()

if __name__ == '__main__':
    argparser = argparse.ArgumentParser(description='Plots data from the raw files output by automatic-tests.py.')
    argparser.add_argument('--input_directory', metavar='I', type=str, help='Directory where the raw output files are.')
    argparser.add_argument('--output_directory', metavar='O', type=str, help='Directory to which the plots will be saved.')
    argparser.add_argument('--endpt_name', metavar='E', type=str, help='Substring present in name of destination endpoint.')
    cmd_line_args = argparser.parse_args()

    collect_data_from_directory(cmd_line_args.input_directory, cmd_line_args.endpt_name, cmd_line_args.output_directory)
    #analyse_remy_with_kernel(cmd_line_args.input_directory, endpt_name=cmd_line_args.endpt_name)

    #data_folder = '../raw-results/UCLA-MIT/with-onoff-cubics/'
    # data_folder = '../raw-results/fig4-reproduce/'