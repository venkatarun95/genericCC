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
 	_intersend_time = initial_intersend_time;
	prev_ack_sent_time = 0.0;

	slow_start = true;

	_the_window = 10; //numeric_limits<double>::max();
	_timeout = 1000;

	num_pkts_lost = num_pkts_acked = 0;
	
	start_time_point = std::chrono::high_resolution_clock::now();
}

void MarkovianCC::do_slow_start() {
	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);
	double cur_time = current_timestamp();

	// Increase until saturation

	static double prev_send_ewma = initial_intersend_time;
	static double prev_update_time = -1.0;
	static unsigned int prev_num_pkts_acked = 0;
	static double prev_prev_send_ewma = 0.0;

	static double sum_inter_recv_time = 0.0;
	static double last_ack_time = 0.0;

	if (num_pkts_acked < 10)
		return;
	if (prev_update_time == -1.0) {
		prev_update_time = cur_time;
		prev_num_pkts_acked = num_pkts_acked;
	}
	if (last_ack_time != 0.0)
		sum_inter_recv_time += cur_time - last_ack_time;
	last_ack_time = cur_time;

	if (prev_update_time > cur_time - rtt_ewma)
		return;
	double prev_recv_ewma = sum_inter_recv_time / (num_pkts_acked - prev_num_pkts_acked);//(cur_time - prev_update_time) / (num_pkts_acked - prev_num_pkts_acked);
	if (prev_recv_ewma > 2*prev_prev_send_ewma && prev_prev_send_ewma != 0) {
		//slow_start = false;
		// _intersend_time *= 2;
		prev_send_ewma = 100.0;
		prev_update_time = 0.0;
		prev_num_pkts_acked = 0;
		sum_inter_recv_time = 0.0;
		// _intersend_time = prev_recv_ewma;
		slow_start = false;
		_intersend_time *= 2;
		_the_window /= 2;
		intersend_ewma.force_set(_intersend_time, cur_time / min_rtt);
		// cout << "Stopped slow start: " << prev_recv_ewma << " " << prev_prev_send_ewma << " " << _intersend_time << endl;
		return;
	}
	// if (prev_num_pkts_acked - num_pkts_acked > 0.9*_the_window)
	// 	_the_window *= 2;
	_the_window = rtt_ewma / (_intersend_time);
	_intersend_time /= 2;
	// cout << "Slow Start double: " << _intersend_time << " " << _the_window << " " << prev_recv_ewma << endl;
	prev_prev_send_ewma = prev_send_ewma;
	prev_send_ewma = _intersend_time;
	prev_update_time = cur_time;
	prev_num_pkts_acked = num_pkts_acked;
	sum_inter_recv_time = 0.0;


	// Guess number of senders

	// _the_window = numeric_limits<double>::max();
	// static double start_time = 0;
	// const double link_rate = 2.667;
	// if (num_pkts_acked < 10)
	// 	return;
	// if (start_time == 0) {
	// 	start_time = cur_time;
	// 	cout << rtt_ewma << " " << rtt_ewma - min_rtt << endl;
	// 	// double est_num_senders = link_rate * delta * 1.0/(1.0/(rtt_ewma - min_rtt) + 1/_intersend_time) - delta;
	// 	// _intersend_time = (est_num_senders + 1) / link_rate;
	// 	// cout << est_num_senders << " " << _intersend_time << " " << (1.0/(rtt_ewma - min_rtt) + 1/_intersend_time) << endl;

	// 	double tmp_spare_rate = 1.0 / (rtt_ewma - min_rtt) + 1.0 / _intersend_time;
	// 	double est_num_senders = delta * (link_rate - tmp_spare_rate) / tmp_spare_rate ;
	// 	_intersend_time = (est_num_senders + 1) / link_rate;
	// 	cout << _intersend_time << " " << est_num_senders << " " << tmp_spare_rate << endl;
	// }
	// if (cur_time - start_time > rtt_ewma) {
	// 	slow_start = false;
	// 	start_time = 0;
	// 	intersend_ewma.force_set(_intersend_time, cur_time / min_rtt);
	// 	cout << "Slow start stopped" << endl;
	// }
}

void MarkovianCC::update_intersend_time() {
	double cur_time = current_timestamp();
	if (!intersend_ewma.is_valid())
	 	return;

	if (slow_start) {
		do_slow_start();
		return;
	}

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
	double new_intersend_mean = (delta + 1) / (1.0/(rtt_ewma - min_rtt) + 1.0/intersend_ewma);
	// double new_intersend_mean = delta * (rtt_ewma - min_rtt);

	// double new_intersend_mean = delta * (rtt_ewma - min_rtt + 1.0) / 10.0;
	// double new_intersend_mean = (delta + 1) / (0.376/(rtt_ewma - min_rtt + 1.0) + 1.0/intersend_ewma);
	// cout << new_intersend_mean << " " << rtt_ewma - min_rtt << endl;

	if (num_pkts_acked < 10) {
	 	_intersend_time = max(new_intersend_mean, tmp_old_intersend_time); // to avoid bursts due tp min_rtt updates
	 	mean_intersend_time.update(new_intersend_mean, cur_time / min_rtt);
	 	return;
	}

	mean_intersend_time.update(new_intersend_mean, cur_time / min_rtt);

	Exponential distr(1.0/mean_intersend_time, prng);
	_intersend_time = distr.sample();
	// _intersend_time = mean_intersend_time;
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

	// if (_the_window < numeric_limits<int>::max())
	// 	_the_window = numeric_limits<int>::max();

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