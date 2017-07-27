#include "remycc.hh"

double RemyCC::current_timestamp( void ){
	return cur_tick;
}

void RemyCC::init( void ){
	flow_id = 0;
	start_time_point = std::chrono::high_resolution_clock::now();
	rat.reset( current_timestamp() );
	_the_window = 2.0;
	_intersend_time = 0;
}

void RemyCC::onACK(int ack, double receiver_timestamp, double sender_timestamp __attribute((unused))){
	int seq_num = ack - 1;
	//assert( unacknowledged_packets.count( seq_num ) > 0);
	if ( unacknowledged_packets.count( seq_num ) > 1 ) { std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) { std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

	double sent_time = unacknowledged_packets[seq_num];
	unacknowledged_packets.erase(seq_num);
	
	Packet p ( 0, flow_id, sent_time, seq_num );
	p.tick_received = current_timestamp();
	p.receiver_timestamp = receiver_timestamp;
	
	std::vector< Packet > temp_packets ( 1, p );
	
#ifdef SCALE_SEND_RECEIVE_EWMA
	if ( measured_link_rate > 0 ){
		// normalize w.r.t NUM_PACKETS_PER_LINK_RATE_MEASUREMENT because this 
		// function is called only once for each group of NUM_PACKETS_PER_LINK_RATE_MEASUREMENT
		rat.packets_received( temp_packets, 1 * TRAINING_LINK_RATE / measured_link_rate );
		_the_window = rat.cur_window_size() * measured_link_rate / TRAINING_LINK_RATE;
		_intersend_time = rat.cur_intersend_time() * TRAINING_LINK_RATE / measured_link_rate;
	}
	else {
		rat.packets_received( temp_packets, 1.0 );
		_the_window = rat.cur_window_size();
		_intersend_time = rat.cur_intersend_time();
	}
#else
	rat.packets_received( temp_packets, 1.0 );
	_the_window = rat.cur_window_size();
	_intersend_time = rat.cur_intersend_time();
#endif
}

void RemyCC::onLinkRateMeasurement( double s_measured_link_rate ){
	measured_link_rate = s_measured_link_rate;
}

void RemyCC::onPktSent(int seq_num){
	unacknowledged_packets[seq_num] = current_timestamp();
}
