#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;

class KernelTCP{

	string dstaddr;
	int dstport;

public:
	KernelTCP( string &ipaddr, int port ) 
	:	dstaddr( ipaddr ),
		dstport( port )
	{ }

	// takes flow_size in milliseconds (byte_switched=false) or in bytes (byte_switched=true) 
	void send_data( double flow_size, bool byte_switched, int flow_id __attribute((unused)), int src_id __attribute((unused)) ){
		//string command = "sockperf ul --tcp --mps=max --msg-siz.e=1472 --ip " + dstaddr + " --port " + to_string( dstport ) + " --time " + to_string( int( duration/1000 ) ) + " --dontwarmup " + " >> ./kernel-tcp-out.txt 2>&1";
		string command;
		if(!byte_switched)
			command = "iperf -c " + dstaddr + " -t " + to_string( flow_size/1000.0 ).c_str();
		else
			command = "iperf -c " + dstaddr + " -n " + to_string( flow_size ).c_str();
		int res = system( command.c_str() );
		//int res2 = system( "python parse-sockperf.py < /tmp/kernel-tcp-out" ); res2 = res2;
		std::cout<<"Ran \""<<command<<"\" Returned with: "<<res<<std::endl;
	}
};