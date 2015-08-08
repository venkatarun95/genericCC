#include "nashcc.hh"

#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

double NashCC::current_timestamp(void) {
	using namespace std::chrono;
	high_resolution_clock::time_point cur_time_point = \
		high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point)\
		.count()*1000; 
}

void NashCC::init() {
	if (num_pkts_acked != 0)
		cout << "%% Packets lost: " << (100.0*num_pkts_lost)/(num_pkts_acked+num_pkts_lost) << endl;
	unacknowledged_packets.clear();

	// check note in onPktSent before changing
	min_rtt = numeric_limits<double>::max(); 

	rtt_acked_ewma.reset();
	rtt_unacked_ewma.reset();
	intersend_ewma.reset();
 	_intersend_time = initial_intersend_time;
	prev_ack_sent_time = 0.0;

	_the_window = numeric_limits<int>::max();//10;
	_timeout = 1000;

	num_pkts_lost = num_pkts_acked = 0;
	
	start_time_point = std::chrono::high_resolution_clock::now();
}

void NashCC::update_intersend_time(double cur_time) {
	bool fast_start_done = false; //set to true after fast start has been performed
	if (!intersend_ewma.is_valid())
	 	return;//_intersend_time = (delta + 1) / (1.0/rtt_ewma);
	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);
	if (rtt_ewma == 0.0)
		return; // can happen for first few packets as min_rtt estimate is not precise

	double tmp = _intersend_time;
	_intersend_time = (delta + 1) / (1.0/rtt_ewma + 1.0/intersend_ewma);

	if (num_pkts_acked < 10)
	 	_intersend_time = max(_intersend_time, tmp); // to avoid bursts due tp min_rtt updates
	if (!fast_start_done) { // fast start
		_intersend_time = max(delta, 1.0) * rtt_ewma;
		intersend_ewma.reset();
		intersend_ewma.update(_intersend_time, cur_time / min_rtt);
		intersend_ewma.round();
		fast_start_done = true;
	}
}

void NashCC::delta_update_max_delay(double rtt, double cur_time) {
	assert(mode == UtilityMode::MAX_DELAY);
	static double last_delta_update_time = 0.0;
	static double average_rtt = 0.0;
	static unsigned int num_rtt_measurements = 0;
	static double last_delta_update = 0;

	if (num_pkts_acked < 10)
		return;

	if (last_delta_update_time == 0.0)
		last_delta_update_time = cur_time;

	average_rtt += rtt;
	++ num_rtt_measurements;

	if (last_delta_update_time < cur_time - 2*rtt) {
		last_delta_update_time = cur_time;
		average_rtt /= num_rtt_measurements;

		if (average_rtt < params.max_delay.queueing_delay_limit) {
			last_delta_update -= 0.1;
		}
		else {
			last_delta_update += 0.1;
		}
		delta += last_delta_update;

		delta = max(0.01, delta);

		num_rtt_measurements = 0;
		average_rtt = 0.0;
	}
}

void NashCC::onACK(int ack, double receiver_timestamp __attribute((unused))) {
	int seq_num = ack - 1;
	
	// some error checking
	if ( unacknowledged_packets.count(seq_num) > 1 ) { 
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count(seq_num) < 1 ) { 
		std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

	double sent_time = unacknowledged_packets[seq_num];
	double cur_time = current_timestamp();

	// update min rtt
	if (cur_time - sent_time < min_rtt) {
		if (min_rtt != numeric_limits<double>::max()) {
			rtt_acked_ewma.add( min_rtt - (cur_time - sent_time) );
			rtt_unacked_ewma.add( min_rtt - (cur_time - sent_time) );
		}
		min_rtt = cur_time - sent_time;
	}

	// update rtt_acked_ewma
	rtt_acked_ewma.update((cur_time - sent_time - min_rtt) / min_rtt, 
		cur_time/min_rtt);
	rtt_acked_ewma.round();

	// update intersend_ewma
	if (prev_ack_sent_time != 0.0) { 
		intersend_ewma.update(sent_time - prev_ack_sent_time, cur_time / min_rtt);
		intersend_ewma.round();
		update_intersend_time(cur_time);
	}	
	prev_ack_sent_time = sent_time;

	// adjust delta
	if (mode == UtilityMode::MAX_DELAY) {
		delta_update_max_delay(cur_time - sent_time, cur_time);
	}

	// delete this pkt and any unacknowledged pkts before this pkt
	for (auto x : unacknowledged_packets) {
		if(x.first > seq_num)
			break;
		if(x.first < seq_num) {
			++ num_pkts_lost;
		}
		unacknowledged_packets.erase(x.first);
	}
	++ num_pkts_acked;

	if (_the_window < numeric_limits<int>::max())
		_the_window = numeric_limits<int>::max();
}

void NashCC::onLinkRateMeasurement( double s_measured_link_rate __attribute((unused)) ) {
	assert( false );
}

void NashCC::onPktSent(int seq_num) {
	// add to list of unacknowledged packets
	assert( unacknowledged_packets.count( seq_num ) == 0 );
	double cur_time = current_timestamp();
	unacknowledged_packets[seq_num] = cur_time;

	// check if rtt_ewma is to be increased
	rtt_unacked_ewma = rtt_acked_ewma;
	for (auto & x : unacknowledged_packets) {
		double rtt_lower_bound = cur_time - x.second - min_rtt;
		if (min_rtt == numeric_limits<double>::max())
			rtt_lower_bound = cur_time - x.second;
		if (rtt_lower_bound <= rtt_unacked_ewma)
			break;
		rtt_unacked_ewma.update(rtt_lower_bound, cur_time);
	}
	rtt_unacked_ewma.round();
}