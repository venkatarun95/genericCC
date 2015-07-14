import os
import re

folder_path = "../range-test-results/UCLA-MIT/without-normalization/remy-broad-trained-with-normalization"

re_num = re.compile(".*\s([0-9.e+]+)\.*")
re_num_senders = re.compile('.*[^0-9]([0-9]+)')
re_transmission_time = re.compile('.*Transmitted for ([0-9.]+) .*')

outputs = []

def parse_file(filename, print_csv=False):
  """Return a list of dictionaries.

    Keyword arguments:
    filename -- full/relative path
    print_csv -- whether or not to print data in csv format
   
    Each element in list corresponds to one connection.

    Each dictionary contains various parameters of interest such as 
    throughput, average RTT, transmission time (sec) etc.
  """
  connections = [] # to be returned

  interesting_properties = [
    'Throughput',
    'RTT',
    'NumBytes',
    'NumPkts',
    'TransmitTime',
  ]

  infile = open(filename, 'r')
  line = infile.readline()

  num_pts, num_invalid_pts = 0, 0

  while line != "":
    if line.find('Data Successfully Transmitted') != -1:
      try:
        line = infile.readline()
        throughput = float( re_num.match( line ).group( 1 ) )
        line = infile.readline()
        rtt = float( re_num.match( line ).group( 1 ) )

        line = infile.readline()
        line = infile.readline()
        line = infile.readline()
        line = infile.readline()

        transmit_time = float(re_transmission_time.match(line).group( 1 ))/1000

      except:
        num_invalid_pts += 1
        continue

      connections.append({
        'Throughput': throughput,
        'RTT': rtt * 1000,
        'NumBytes': throughput*transmit_time,
        'NumPkts': throughput*transmit_time/1500.0,
        'TransmitTime': transmit_time,
      })
    else:
      line = infile.readline()

  #print num_invalid_pts, "invalid points."
  
  if print_csv:
    res = ""
    for title in interesting_properties:
      res += title + ", "
    print res[:-2]

    for conn in connections:
      res = ""
      for title in interesting_properties:
        res += str(conn[title]) + ", "
      print res[:-2]
  return connections

if __name__ == '__main__':
  res = parse_file('../raw-results/UCLA-MIT/remy-default_link1-20_rtt50-200_16senders_normalized.dna.4-8')