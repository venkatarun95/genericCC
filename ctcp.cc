/*#include <string.h>
#include <stdio.h>

#include "ctcp.hh"

using namespace std;

double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
	using namespace chrono;
	high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

template<class T>
void CTCP<T>::send_data ( double duration, int flow_id, int src_id ){
	TCPHeader header, ack_header;

	char buf[packet_size];
	memset(buf, '-', sizeof(char)*packet_size);
	buf[packet_size-1] = '\0';

	chrono::high_resolution_clock::time_point start_time_point = chrono::high_resolution_clock::now();
	_last_send_time = 0.0;
	double cur_time = 0;
	int transmitted_bytes = 0;

	int seq_num = 0;
	_last_send_time = 0.0;
	_largest_ack = -1;
	_packets_sent = 0;

	double delay_sum = 0;
	int num_packets_transmitted = 0;

	congctrl.init();

	while ( cur_time < duration ){
		cur_time = current_timestamp( start_time_point );
		assert( _packets_sent >= _largest_ack + 1 );
		while( ( (_packets_sent < _largest_ack + 1 + congctrl.get_the_window()) 
			and (_last_send_time + congctrl.get_intersend_time() <= cur_time) ) 
			and cur_time < duration ){
			
			header.seq_num = seq_num;
			header.flow_id = flow_id;
			header.src_id = src_id;
			header.sender_timestamp = cur_time;

			memcpy(buf, &header, sizeof(TCPHeader));
			congctrl.onPktSent( seq_num );
			socket.senddata( buf, packet_size, NULL );

			cur_time = current_timestamp( start_time_point );
			_packets_sent++;
			_last_send_time = cur_time;
			seq_num += 1;

			cout<<"sent"<<congctrl.get_the_window()<<" ";
		}

		cur_time = current_timestamp( start_time_point );
		double timeout = _last_send_time + congctrl.get_timeout(); // everything in milliseconds
		if( congctrl.get_the_window() > 0 )
			timeout = min( timeout, _last_send_time + congctrl.get_intersend_time() - cur_time );

		sockaddr_in other_addr;
		if( socket.receivedata( buf, packet_size, timeout, other_addr ) == 0 ) {
			cur_time = current_timestamp( start_time_point );
			if( cur_time > _last_send_time + congctrl.get_timeout() )
				congctrl.onTimeout();
			continue;
		}

		memcpy(&ack_header, buf, sizeof(TCPHeader));
		ack_header.seq_num ++; //because the receiver doesn't do that for us yet

		if ( ack_header.src_id != src_id || ack_header.flow_id != flow_id ){
			if( ack_header.src_id != src_id ){
				std::cerr<<"Received incorrect ack for src "<<ack_header.src_id<<" to "<<src_id<<endl;
			}
			continue;
		}
		
		_largest_ack = max(_largest_ack, ack_header.seq_num);
		congctrl.onACK(seq_num);
		cout<<"acked";

		// Track statistics
		cur_time = current_timestamp( start_time_point );
		delay_sum += cur_time - ack_header.sender_timestamp;
		this->tot_delay += cur_time - ack_header.sender_timestamp;

		transmitted_bytes += data_size;
		this->tot_bytes_transmitted += data_size;

		num_packets_transmitted += 1;
		this->tot_packets_transmitted += 1;
	}

	congctrl.close();

	cur_time = current_timestamp( start_time_point );
	this->tot_time_transmitted += cur_time;

	double throughput = transmitted_bytes/( cur_time / 1000.0 );
	double delay = (delay_sum / 1000) / num_packets_transmitted;

	std::cout<<"\n\nData Successfully Transmitted\n\tThroughput: "<<throughput<<" bytes/sec\n\tAverage Delay: "<<delay<<" sec/packet\n";

	double avg_throughput = tot_bytes_transmitted / ( tot_time_transmitted / 1000.0);
	double avg_delay = (tot_delay / 1000) / tot_packets_transmitted;
	std::cout<<"\n\tAvg. Throughput: "<<avg_throughput<<" bytes/sec\n\tAverage Delay: "<<avg_delay<<" sec/packet\n";
}

template<class T>
void CTCP<T>::listen_for_data ( ){

}*/