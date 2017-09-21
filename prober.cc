#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string.h>
#include <thread>

#include "tcp-header.hh"
#include "udp-socket.hh"

#define NUM_PACKETS_PER_LINK_RATE_MEASUREMENT (30)
// the lower the value, the slower the exponential averaging
#define LINK_RATE_MEASUREMENT_ALPHA (1)

const int num_packets_per_link_rate_measurement = NUM_PACKETS_PER_LINK_RATE_MEASUREMENT;
const double link_rate_measurement_alpha = LINK_RATE_MEASUREMENT_ALPHA;

using namespace std;

bool LINK_LOGGING = true;
string LINK_LOGGING_FILENAME ;

double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
	using namespace chrono;
	high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

int main( int argc, char* argv[] ) {
	string dstaddr;
	int dstport;
	if( argc != 4 ){
		cout << "Usage: ./prober dstaddr dstport outfilename " << endl;
		exit( 0 );
	}
	dstaddr = argv[1];
	dstport = atoi( argv[2] );
	LINK_LOGGING_FILENAME = argv[3];

	UDPSocket socket;
	socket.bindsocket( dstaddr, dstport, 0 );

	const int src_id = 42; // some arbitrary number
	const int packet_size = sizeof(TCPHeader)+2;
	TCPHeader header, ack_header;

	// this is the data that is transmitted. A sizeof(TCPHeader) header followed by a sring of dashes
	char buf[packet_size];
	memset(buf, '-', sizeof(char)*packet_size);
	buf[packet_size-1] = '\0';

	// for link logging
	ofstream link_logfile;
	if( LINK_LOGGING )
		link_logfile.open( LINK_LOGGING_FILENAME, ios::out | ios::app );

	// for flow control
	double cur_time = 0;
	// note: this is not the sequence number that is actually transmitted. packets are transmitted in groups of num_packets_per_link_rate_measurement (say n) numbered as seq_num*n, seq_num*n + 1, ..., seq_num*n + (n-1)
	int seq_num = 0;

	// variables for link rate measurement
	int cur_ack_group_number = -1;
	int num_packets_received_in_current_group = -1;
	double latest_ack_time_in_group = -1;
	double link_rate_measurement_accumulator = -1; // for accumulating before finding average
	double last_measured_link_rate = -1; // in packets/s. In case num_packets_per_link_rate_measurement == 1, this value will be maintained as -1.

	chrono::high_resolution_clock::time_point start_time_point = chrono::high_resolution_clock::now();

	while ( true ) {
		cur_time = current_timestamp( start_time_point );
		
		for (  int i = 0;i < num_packets_per_link_rate_measurement; i++ ) {
			header.seq_num = seq_num * num_packets_per_link_rate_measurement + i;
			header.flow_id = 0;
			header.src_id = src_id;
			header.sender_timestamp = cur_time;
			header.receiver_timestamp = 0;

			memcpy( buf, &header, sizeof(TCPHeader) );
			socket.senddata( buf, packet_size, NULL );

			cur_time = current_timestamp( start_time_point );
		}

		seq_num++; // a set of num_packets_per_link_rate_measurement (say 3) are sequence numbered as 3n, 3n+1, 3n+2. This is later used for link rate measurement while receiving packets.
		
		for( int i = 0;i < num_packets_per_link_rate_measurement; i++ ) {
			sockaddr_in other_addr;
			if( socket.receivedata( buf, packet_size, -1, other_addr ) == 0 ) {
				cur_time = current_timestamp( start_time_point );
        --i;
        continue;
				//assert( false );
			}

			memcpy(&ack_header, buf, sizeof(TCPHeader));
			ack_header.seq_num ++; // because the receiver doesn't do that for us yet

			if ( ack_header.src_id != src_id || ack_header.flow_id != 0 ) {
				if( ack_header.src_id != src_id ) {
					std::cerr<<"Received incorrect ack for src "<<ack_header.src_id<<" to "<<src_id<<endl;
				}
				continue;
			}

			cur_time = current_timestamp( start_time_point );

			// measure link speed
			assert( num_packets_per_link_rate_measurement > 0 );
			if ( num_packets_per_link_rate_measurement > 1) {
				int ack_group_num = ( ack_header.seq_num - 1) / num_packets_per_link_rate_measurement;
				int ack_group_seq = ( ack_header.seq_num - 1) % num_packets_per_link_rate_measurement;
				// Note: in case of reordering, the measurement is cancelled. Measurement will be bad in case network reorders extensively
				if ( ack_group_num == cur_ack_group_number ) { 
					if ( ack_group_seq == num_packets_received_in_current_group ) {
						num_packets_received_in_current_group += 1;
						link_rate_measurement_accumulator += cur_time - latest_ack_time_in_group;
						latest_ack_time_in_group = cur_time;
						if( num_packets_received_in_current_group == num_packets_per_link_rate_measurement ) {
							if ( last_measured_link_rate < 0 ) // set initial value
								last_measured_link_rate = ( 1000 / ( link_rate_measurement_accumulator / (num_packets_per_link_rate_measurement - 1) ) );
							else
								last_measured_link_rate = last_measured_link_rate*(1 - link_rate_measurement_alpha) + link_rate_measurement_alpha*( 1000 / ( link_rate_measurement_accumulator / (num_packets_per_link_rate_measurement - 1) ) );
							
							// Log measured link speed and last RTT
							if( LINK_LOGGING )
								link_logfile << cur_time << " " << last_measured_link_rate << " " << cur_time - ack_header.sender_timestamp << endl << flush;
						}
					}
				}
				else if ( ack_group_num > cur_ack_group_number ) {
					cur_ack_group_number = ack_group_num;
					num_packets_received_in_current_group = 1;
					link_rate_measurement_accumulator = 0;
					latest_ack_time_in_group = cur_time;
				}
			}
		}

		this_thread::sleep_for( chrono::seconds( 1 ) );
	}

	if( LINK_LOGGING )
		link_logfile.close();
}
