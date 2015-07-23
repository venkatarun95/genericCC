import argparse
import os
import random
import re
import time

rat_names = []
deltas = [10, 1]

num_senders_list = [32, 48] #[1, 4, 8, 16]
num_kernels_list = [0]

def discover_rats(directory):
    global rat_names
    if not os.path.isdir(directory):
        print("Could not open '" + directory + "'. Please specify a directory.")
        exit(1)
    try:        
        rat_names = [ os.path.join(directory, f) for f in os.listdir(directory) if os.path.isfile(
            os.path.join(directory, f)) ]
    except:
        print("Error while reading from directory: '" + directory + "'.")

re_rat_name = re.compile('.*-linkppt([0-9.]+)\.dna.*')
def run_test(cctype, rat_id, delta, num_senders, num_kernels, cmd_line_args):
    global rat_names

    #Note: Usage: ./run-senders-parallel: source_interface serverip serverport cctype ratname delta numsenders linkppt numkernels outfilename
    if cctype == 'kernel':
        serverport = '5001'
        rat_id = 0 # some default. The 'sender' program will ignore it.
        num_senders += num_kernels
        num_kernels = 0
    elif cctype == 'remy':
        serverport = '8888'
    elif cctype == 'nash':
        rat_id = 0
        serverport = '8888'
    else:
        assert(False)

    ratname_nice = ('norat', rat_names[rat_id].split('/')[-1])[cctype=='remy']
    cctype_embellish = ('', str(delta))[cctype=='nash']

    out_filename = os.path.join(cmd_line_args.raw_output_folder, 
        cctype + cctype_embellish + '-' + ratname_nice + '-' 
        + str(num_kernels) + '-' + str(num_senders))
    in_filename = rat_names[rat_id]

    linkppt = re_rat_name.match(in_filename).group(1)
    
    runstr = 'sudo ./run-senders-parallel.sh ' + cmd_line_args.source_interface \
             + ' ' + cmd_line_args.server_ip + ' ' + serverport + ' ' + cctype \
             + ' ' + in_filename + ' ' + str(delta) + ' '+ str(num_senders) \
             + ' ' + linkppt + ' ' + str(num_kernels) + ' ' + out_filename
    print runstr
    os.system(runstr)
    
    time.sleep(5) # wait for queues to clear

if __name__ == '__main__':
    argparser = argparse.ArgumentParser()#description='Run different CC protocols for 100 seconds at a time, choosing among them randomly.')
    argparser.add_argument('--input_rats', type=str, help='Directory where the dna files to be tested are stored')
    argparser.add_argument('--raw_output_folder', type=str, help='Directory where raw results are to be written')
    argparser.add_argument('--server_ip', type=str, help="IP address od the server")
    argparser.add_argument('--source_interface', type=str, help='Interface from which to send. Defaults to eth0')

    cmd_line_args = argparser.parse_args()

    discover_rats( cmd_line_args.input_rats)

    random.seed()
    while True:
        x = random.randrange(len(rat_names)+len(deltas)+1)
        rat_id, delta = 0, 0.0
        print "Running only Kernel" + str(x)
        if x < len(deltas): # everybody has equal probability
            cctype = 'nash'
            delta = deltas[random.randrange(len(deltas))]
        elif x == len(deltas): 
            cctype = 'kernel'
            continue
        else:
            cctype = 'remy'
            continue
            rat_id = random.randrange(len(rat_names))
        num_senders = num_senders_list[ random.randrange(len(num_senders_list)) ]
        num_kernels = num_kernels_list[ random.randrange(len(num_kernels_list)) ]

        print "Running '" + cctype + "' with " + str(num_senders) + " senders" + (" with '" + rat_names[rat_id] + "'", "")[cctype != 'remy'] + (" with delta=" + str(delta), "")[cctype != 'nash']

        run_test(cctype, rat_id, delta, num_senders, num_kernels, cmd_line_args)
