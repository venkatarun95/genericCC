import os
import re

def parse_file(filename, endpt_name="csail.mit.edu", dst_endpt_name="csail.mit.edu", print_csv=False):
  """Return a list of dictionaries.

    Keyword arguments:
    filename -- full/relative path
    endpt_name -- substring to be found in endpoint names for exactly 
      the connections we are interested in
    dst_endpt_name -- statistics collected will be for this endpt as dst
    print_csv -- whether or not to print data in csv format
   
    Each element in list corresponds to one TCP connection, note that a
    TCP connection is only considered if we are interested in it 
    (decided based on endpoints).

    Each dictionary contains various parameters of interest such as 
    throughput, average RTT, num packets sent etc.
   """

  interesting_properties = [
    'throughput',
    'RTT avg',
    'actual data bytes',
    'actual data pkts',
    'data xmit time'
    ]

  title_rename_table = {'throughput': 'Throughput',
                        'RTT avg': 'RTT',
                        'actual data bytes': 'NumBytes',
                        'actual data pkts': 'NumPkts',
                        'data xmit time': 'TransmitTime'}

  re_interesting_properties = [
    re.compile('\s*'+x+':\s*([\d.]+)[\D\s]+([\d.]+).*') 
    for x in interesting_properties
    ]

  connections = [] # list to be returned as described in the docstring
  state = 0 # 0-searching for a TCP connection, 1-looking for values

  infile = open(filename, 'r')

  while True:
    line = infile.readline()
    if not line: break

    if line.find('TCP connection') != -1:
      if state == 1:
        connections.append(conn_details)
      if infile.readline().find(endpt_name) != -1: 
        state, conn_details, conn_dir = 1, {}, 1
      elif infile.readline().find(endpt_name) != -1:
        state, conn_details, conn_dir = 1, {}, 0
      else:
        state = 0

    if state == 0:
      continue

    for i in range(len(interesting_properties)):
      prop, re_prop = interesting_properties[i], re_interesting_properties[i]
      if line.find(prop + ':') != -1:
        prop = title_rename_table[prop]
        assert(prop not in conn_details)
        try:
          conn_details[prop] = float( re_prop.match(line).groups()[conn_dir] )
        except AttributeError: #posssibly because of 'NA' appearing in 'line'
          conn_details[prop] = 0.0
          state = 0 #skip this connection
          print "Warning: Skipping a connection"

  if print_csv:
    res = ""
    for title in title_rename_table:
      res += title_rename_table[title] + ", "
    print res[:-2]

    for conn in connections:
      res = ""
      for title in title_rename_table:
        res += str(conn[title_rename_table[title]]) + ", "
      print res[:-2]
  return connections

if __name__ == '__main__':
  #res = parse_file('../raw-results/UCLA-MIT/kernel-bigbertha-10x.dna.4-2-tcptrace')
  res = parse_file('../hello-tcptrace', '100.64.0.1', '100.64.0.1')