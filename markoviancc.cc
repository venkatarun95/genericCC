#include "markoviancc.hh"

#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

double MarkovianCC::current_timestamp( void ){
	using namespace std::chrono;
	high_resolution_clock::time_point cur_time_point = \
		high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point)\
		.count()*1000; 
}

void MarkovianCC::init() {
	if (num_pkts_acked != 0)
		cout << "%% Packets lost: " << (100.0*num_pkts_lost)/(num_pkts_acked+num_pkts_lost) << endl;
	unacknowledged_packets.clear();

	min_rtt = numeric_limits<double>::max();

	mean_intersend_time.reset();

	rtt_acked_ewma.reset();
	rtt_unacked_ewma.reset();
	intersend_ewma.reset();
 	_intersend_time = 10;
	prev_ack_sent_time = 0.0;

	_the_window = numeric_limits<int>::max();//10;
	_timeout = 1000;

	num_pkts_lost = num_pkts_acked = 0;
	
	start_time_point = std::chrono::high_resolution_clock::now();
}

void MarkovianCC::update_intersend_time() {
	double cur_time = current_timestamp();
	if (!intersend_ewma.is_valid())
	 	return;
	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);

	double tmp_old_intersend_time = _intersend_time;
	// _intersend_time = delta * (rtt_ewma - min_rtt);
	// cout << _intersend_time << " " << rtt_ewma << endl;
	// represents \mu - \overline{lambda_i}

	// Utility = throughput / delay^\delta
	// double tmp_spare_rate = 1.0 / (rtt_ewma - min_rtt) + 1/intersend_ewma;
	// _intersend_time = 2*min_rtt / (delta+1 + 2*min_rtt*tmp_spare_rate - 
	// 	sqrt((delta+1)*(delta+1) + 4*delta*min_rtt*tmp_spare_rate));
	
	// Utility = throughput / (queuing delay)^\delta
	// double new_intersend_mean = delta * (rtt_ewma - min_rtt);
	// double new_intersend_mean = (delta + 1) / (1.0/(rtt_ewma - min_rtt) + 1.0/intersend_ewma);
	double new_intersend_mean = delta * (rtt_ewma - min_rtt);
	// cout << _intersend_time << " " << rtt_ewma - min_rtt << " " << rtt_unacked_ewma << " " << rtt_acked_ewma << " " << min_rtt << endl;

	if (num_pkts_acked < 10) {
	 	_intersend_time = max(new_intersend_mean, tmp_old_intersend_time); // to avoid bursts due tp min_rtt updates
	 	mean_intersend_time.update(new_intersend_mean, cur_time / min_rtt);
	 	return;
	}

	mean_intersend_time.update(new_intersend_mean, cur_time / min_rtt);

	// Exponential distr(1.0/mean_intersend_time, prng);
	// _intersend_time = distr.sample();
	_intersend_time = mean_intersend_time;
}

void MarkovianCC::onACK(int ack, double receiver_timestamp __attribute((unused))) {
	int seq_num = ack - 1;

	// some error checking
	if ( unacknowledged_packets.count( seq_num ) > 1 ) { 
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) { 
		std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

	double sent_time = unacknowledged_packets[seq_num];
	double cur_time = current_timestamp();

	min_rtt = min(min_rtt, cur_time - sent_time);

	// update rtt_acked_ewma
	rtt_acked_ewma.update((cur_time - sent_time), 
		cur_time / min_rtt);
	rtt_acked_ewma.round();

	// update intersend_ewma
	if (prev_ack_sent_time != 0.0) { 
		intersend_ewma.update(sent_time - prev_ack_sent_time, \
			cur_time / min_rtt);
		intersend_ewma.round();
		update_intersend_time();
	}	
	prev_ack_sent_time = sent_time;

	// delete this pkt and any unacknowledged pkts before this pkt
	for (auto x : unacknowledged_packets) {
		if(x.first > seq_num)
			break;
		if(x.first < seq_num) {
			++ num_pkts_lost;
			//cout << "Lost: " << seq_num << " " << x.first << " " << cur_time - sent_time << " " << _intersend_time << endl;
		}
		unacknowledged_packets.erase(x.first);
	}
	++ num_pkts_acked;

	if (_the_window < numeric_limits<int>::max())
		_the_window = numeric_limits<int>::max();

}

void MarkovianCC::onLinkRateMeasurement( double s_measured_link_rate __attribute((unused)) ) {
	assert( false );
}

void MarkovianCC::onPktSent(int seq_num) {
	// add to list of unacknowledged packets
	assert( unacknowledged_packets.count( seq_num ) == 0 );
	double cur_time = current_timestamp();
	unacknowledged_packets[seq_num] = cur_time;

	// check if rtt_ewma is to be increased
	rtt_unacked_ewma = rtt_acked_ewma;
	for (auto & x : unacknowledged_packets) {
		double rtt_lower_bound = cur_time - x.second;
		if (rtt_lower_bound <= rtt_unacked_ewma)
			break;
		rtt_unacked_ewma.update(rtt_lower_bound, cur_time / min_rtt);
	}
	rtt_unacked_ewma.round();
}