#include "remycc.hh"

double RemyCC::current_timestamp( void ){
	using namespace std::chrono;
	high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

void RemyCC::init( void ){
	flow_id = 0;
	start_time_point = std::chrono::high_resolution_clock::now();
	rat.reset( current_timestamp() );
	_the_window = 2.0;
	_intersend_time = 0;
}

void RemyCC::onACK(int  ack){
	int seq_num = ack - 1;
	//assert( unacknowledged_packets.count( seq_num ) > 0);
	if ( unacknowledged_packets.count( seq_num ) > 1 ) {std::cout<<"Dupack: "<<seq_num<<std::endl; return;}
	if ( unacknowledged_packets.count( seq_num ) > 1 ) {std::cout<<"Unknown Ack!! "<<seq_num<<std::endl; return;}

	double sent_time = unacknowledged_packets[seq_num];
	unacknowledged_packets.erase(seq_num);

	Packet p ( 0, flow_id, sent_time, seq_num );
	p.tick_received = current_timestamp();
	
	std::vector< Packet > temp_packets ( 1, p );
	rat.packets_received( temp_packets );

	_the_window = rat.cur_window_size();
	_intersend_time = rat.cur_intersend_time();
}

void RemyCC::onPktSent(int seq_num){
	//assert(false);
	//assert( unacknowledged_packets.count( pkt->m_iSeqNo ) == 0 );
	//unacknowledged_packets.insert( std::make_pair<int, double>( seq_num, current_timestamp() ) );
	unacknowledged_packets[seq_num] = current_timestamp();	
}