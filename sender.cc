#include <chrono>
#include <fcntl.h>

#include "congctrls.hh"
#include "remycc.hh"
#include "ctcp.hh"
#include "kernelTCP.hh"
#include "pcc-tcp.hh"
#include "traffic-generator.hh"

int main( int argc, char *argv[] ){
	WhiskerTree whiskers;
	bool ratFound = false;

	string serverip="", sourceip="";
	int serverport=-1, sourceport=-1;
	int offduration=-1, onduration=-1;

	enum CCType { REMYCC, TCPCC, KERNELCC, PCC } cctype = REMYCC;

	for ( int i = 1; i < argc; i++ ) {
		std::string arg( argv[ i ] );
		if ( arg.substr( 0, 3 ) == "if=" ) {
			std::string filename( arg.substr( 3 ) );
			int fd = open( filename.c_str(), O_RDONLY );
			if ( fd < 0 ) {
				perror( "open" );
				exit( 1 );
			}

			RemyBuffers::WhiskerTree tree;
			if ( !tree.ParseFromFileDescriptor( fd ) ) {
				fprintf( stderr, "Could not parse %s.\n", filename.c_str() );
				exit( 1 );
			}
			whiskers = WhiskerTree( tree );
			ratFound = true;

			fprintf( stdout, "%s\n", whiskers.str().c_str() );

			if ( close( fd ) < 0 ) {
				perror( "close" );
				exit( 1 );
			}
		}
		else if( arg.substr( 0, 9 ) == "serverip=" )
			serverip = arg.substr( 9 );
		else if( arg.substr( 0, 9 ) == "sourceip=" )
			sourceip = arg.substr( 9 );
		else if( arg.substr( 0, 11 ) == "serverport=" )
			serverport = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 11 ) == "sourceport=" )
			sourceport = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 12 ) == "offduration=" )
			offduration	= atoi( arg.substr( 12 ).c_str() );
		else if( arg.substr( 0, 11 ) == "onduration=" )
			onduration = atoi( arg.substr( 11 ).c_str() );
		else if( arg.substr( 0, 7 ) == "cctype="){
			std::string cctype_str = arg.substr( 7 );
			if( cctype_str == "remy" )
				cctype = CCType::REMYCC;
			else if( cctype_str == "tcp" )
				cctype = CCType::TCPCC;
			else if ( cctype_str == "kernel" )
				cctype = CCType::KERNELCC;
			else if ( cctype_str == "pcc" )
				cctype = CCType::PCC;
			else
				fprintf( stderr, "Unrecognised congestion control protocol '%s'.\n", cctype_str.c_str() );
		}
	}

	if ( serverip == "" || sourceip == "" || onduration <= 0 || offduration < 0 || sourceport == -1 || serverport == -1){
		fprintf( stderr, "Usage: sender if=(ratname) serverip=(ipaddr) sourceip=(ipaddr) serverport=(port) sourceport=(port) offduration=(time in ms) onduration=(time in ms)\n");
		exit(1);
	}

	if( not ratFound and cctype == CCType::REMYCC ){
		fprintf( stderr, "Please specify remy specification file using if=<filename>\n" );
		exit(1);
	}

	if( cctype == CCType::REMYCC){
		fprintf( stdout, "Using RemyCC.\n" );
		RemyCC congctrl( whiskers );
		CTCP< RemyCC > connection( congctrl, serverip, serverport, sourceip, sourceport );
		TrafficGenerator<CTCP<RemyCC>> traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if( cctype == CCType::TCPCC ){
		fprintf( stdout, "Using UDT's TCP CC.\n" );
		DefaultCC congctrl;
		CTCP< DefaultCC > connection( congctrl, serverip, serverport, sourceip, sourceport );
		TrafficGenerator< CTCP< DefaultCC > > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if ( cctype == CCType::KERNELCC ){
		fprintf( stdout, "Using the Kernel's TCP using sockperf.\n");
		KernelTCP connection( serverip, serverport );
		TrafficGenerator< KernelTCP > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else if ( cctype == CCType::PCC ){
		fprintf( stdout, "Using PCC.\n");
		PCC_TCP connection( serverip, serverport );
		TrafficGenerator< PCC_TCP > traffic_generator( connection, onduration, offduration, TrafficType::EXPONENTIAL_ON_OFF );
		traffic_generator.spawn_senders( 1 );
	}
	else{
		assert( false );
	}
}