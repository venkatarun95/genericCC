#include "nashcc.hh"

#include <cassert>
#include <limits>

using namespace std;

double NashCC::current_timestamp( void ){
	using namespace std::chrono;
	high_resolution_clock::time_point cur_time_point = \
		high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

void NashCC::init() {
	_intersend_time = 0;
	_the_window = 1;
	_timeout = 1000;
	start_time_point = std::chrono::high_resolution_clock::now();	
	measured_link_rate = 0;
	min_rtt = numeric_limits<double>::max();
	intersend_ewma = 0;
	unacknowledged_packets.clear();
}

void NashCC::onACK(int ack, double receiver_timestamp __attribute((unused))) {
	int seq_num = ack - 1;
	//assert( unacknowledged_packets.count( seq_num ) > 0);
	if ( unacknowledged_packets.count( seq_num ) > 1 ) { 
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) { 
		std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

	double sent_time = unacknowledged_packets[seq_num];
	unacknowledged_packets.erase(seq_num);

	double rtt = current_timestamp() - sent_time;
	min_rtt = min( rtt, min_rtt );
	rtt -= min_rtt;

	if ( measured_link_rate == 0 ) return;
	if (_the_window < numeric_limits<double>::max() ) {
		assert (_the_window < numeric_limits<double>::max()-1e9 );
		_intersend_time = 1000.0/measured_link_rate;//_the_window / rtt; //rough estimate
		intersend_ewma = _intersend_time;
		_the_window = numeric_limits<double>::max();
	}

	double sum_other_senders_rate = \
		measured_link_rate/1000.0 - 1/rtt - 1/intersend_ewma;
	sum_other_senders_rate = max( sum_other_senders_rate, 0.0 );

	//Exponential dist( (delta + 1)/(measured_link_rate/1000.0 - sum_other_senders_rate), prng );
	//_intersend_time = dist.sample();
	_intersend_time = (delta + 1)/(measured_link_rate/1000.0 - sum_other_senders_rate);
	intersend_ewma = (1.0-1/64.0)*intersend_ewma + 1/64.0*(_intersend_time);
	//_intersend_time = intersend_ewma;

	//std::cout << "$" << rtt << "\t" << _intersend_time << "~" << intersend_ewma << "\t\t" << measured_link_rate << "\t" << sum_other_senders_rate << std::endl << std::flush;
}

void NashCC::onLinkRateMeasurement( double s_measured_link_rate ) {
	measured_link_rate = s_measured_link_rate;
}

void NashCC::onPktSent(int seq_num){
	assert( unacknowledged_packets.count( seq_num ) == 0 );
	unacknowledged_packets[seq_num] = current_timestamp();	
}