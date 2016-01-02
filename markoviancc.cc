#include "markoviancc.hh"

// #include "rand-gen.hh"

#include <boost/math/special_functions/fpclassify.hpp>
#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

double MarkovianCC::current_timestamp( void ){
	#ifdef SIMULATION_MODE
	return cur_tick;

	#else
	using namespace std::chrono;
	high_resolution_clock::time_point cur_time_point = \
		high_resolution_clock::now();
	return duration_cast<duration<double>>(cur_time_point - start_time_point)\
		.count()*1000;
	#endif
}

void MarkovianCC::init() {
	if (num_pkts_acked != 0)
		cout << "%% Packets lost: " << (100.0*num_pkts_lost)/(num_pkts_acked+num_pkts_lost) << endl;

	min_rtt = numeric_limits<double>::max();
	
	unacknowledged_packets.clear();

	mean_sending_rate.reset();
	mean_sending_rate.force_set(1/initial_intersend_time, 0);

	rtt_acked_ewma.reset();
	rtt_unacked_ewma.reset();
	intersend_ewma.reset();
 	_intersend_time = initial_intersend_time;
	prev_ack_sent_time = 0.0;
	num_pkts_acked = 0;

	_the_window = num_probe_pkts;//numeric_limits<double>::max();
	_timeout = 1000;


	num_pkts_lost = num_pkts_acked = 0;

	sum_rtt = 0.0;
	cur_delta_class = -1;
	last_delta_update_time = 0.0;
	last_delta_update_seq_num = 0;
	last_sent_seq_num = 0;
	flow_length = 0;

	#ifndef SIMULATION_MODE
		start_time_point = std::chrono::high_resolution_clock::now();
	#endif
}

void MarkovianCC::update_delta() {
	if (utility_mode == DEL_FCT) {
		// For min. FCT. Set at the beginning of flow. cur_delta_class = -1
		// in the beginning and flow_length != 0  when we know the flow 
		// length (or when flow is flow is over and we don't care anyway).
		if (cur_delta_class == -1 && flow_length != 0) {
			int best_delta_class = 0;
			double best_fct_estimate = numeric_limits<double>::max();
	 
			for (unsigned i = 0; i < delta_classes.size(); i++) {
				double fct_estimate = flow_length / delta_data[i].avg_tpt
					+ delta_data[i].avg_rtt;
				if (delta_data[i].avg_tpt == 0.0) // No data yet.
					fct_estimate = 0;
				cout << delta_classes[i] << ":" << fct_estimate << ":" << delta_data[i].avg_tpt << ":" << delta_data[i].avg_rtt << " " ;
				if (fct_estimate < best_fct_estimate) {
					best_fct_estimate = fct_estimate;
					best_delta_class = i;
				}
			}
			cout << "$" << endl;
			cur_delta_class = best_delta_class;
			delta = delta_classes[cur_delta_class];
			cout << "Set delta to: " << delta << " " << flow_length << endl;
		}
		// About to close flow
		if (flow_length == 1)
			cur_delta_class = -1;
	}
	else if (utility_mode == PFABRIC_FCT) {
		delta = 0.05 + 1.0 * (1.0 - exp(-1.0 * 1e-5 * flow_length));
		cur_delta_class = 0;
	}
	else if (utility_mode == BOUNDED_DELAY) {
		double cur_time = current_timestamp();
		if (cur_time < last_delta_update_time + 1)
			return; // update every few milliseconds

		double rtt_ewma = max(rtt_acked_ewma, rtt_unacked_ewma); 
		double old_delta = delta; 
		if (rtt_ewma > 0.1) 
			cur_delta_class ++; 
		else
			cur_delta_class --; 
		cur_delta_class = max(0, min((int)delta_classes.size() - 1, cur_delta_class)); 
		delta =	delta_classes[cur_delta_class]; 
		cout << "New delta: " << delta << " " << max(rtt_acked_ewma, rtt_unacked_ewma);
		last_delta_update_time = cur_time; 
		if (delta != old_delta)
			last_delta_update_seq_num = last_sent_seq_num;
	}
	else if (utility_mode == CONSTANT_DELTA)
		; // Do nothing
	else
		assert(false);
}

void MarkovianCC::update_intersend_time() {
	double cur_time = current_timestamp();
	if (!intersend_ewma.is_valid())
	 	return;

	//cout << flow_length << endl;
	update_delta();

	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);

	//	double tmp_old_intersend_time = _intersend_time;

	//cout << rtt_ewma << " " << min_rtt << " " << delta << endl << flush;

	double spare_rate = 1.0 / (rtt_ewma - min_rtt);
	double new_sending_rate = spare_rate / delta;
	//double new_sending_rate = (spare_rate + 1.0 / intersend_ewma) / (delta + 1);

	//cout << new_sending_rate << " " << endl << flush;

	if (num_pkts_acked < num_probe_pkts) {
	 	//_intersend_time = max(1.0 / new_sending_rateding_rate, tmp_old_intersend_time); // to avoid bursts due tp min_rtt updates
	 	mean_sending_rate.update(new_sending_rate, cur_time / min_rtt);
	 	return;
	}

	_the_window = numeric_limits<double>::max();

	// cout << "B " << cur_time << " " << rtt_ewma - min_rtt << " " << spare_rate / delta << " " << min_rtt << endl;

	mean_sending_rate.update(new_sending_rate, cur_time / min_rtt);

	_intersend_time = 1/new_sending_rate;
	//_intersend_time = rand_gen.exponential(1.0 / mean_sending_rate);
	//_intersend_time = rand_gen.normal(1.0 / mean_sending_rate, 0.1 / mean_sending_rate);

	assert(_intersend_time > 0);
}

void MarkovianCC::onACK(int ack, double receiver_timestamp __attribute((unused)), double sent_time) {
	int seq_num = ack - 1;
	double cur_time = current_timestamp();

	// Don't use feedback from packets sent on a previous delta
	if (seq_num < last_delta_update_seq_num) {
		// I think this never happens because tcp.h reorders packets before
		// calling these functions.
		//cout << "Ignoring ack" << endl;
		//assert(false);
		num_pkts_acked ++;
		return;
	}

	// some error checking
	if ( unacknowledged_packets.count( seq_num ) > 1 ) {
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) {
		min_rtt = min(min_rtt, cur_time - sent_time);
		num_pkts_acked ++;
		rtt_acked_ewma.update((cur_time - sent_time),
			cur_time / min_rtt);
		rtt_acked_ewma.round();
		update_intersend_time();
		std::cerr<<"Unknown Ack!! "<<seq_num<< " " << min_rtt << " " << num_pkts_acked << ::endl;
		return;
	}

	//assert(num_pkts_acked < 100 || min_rtt <= cur_time - sent_time);

	if (min_rtt > numeric_limits<double>::max() / 2.0) {
		rtt_acked_ewma.force_set(cur_time - sent_time, cur_time / min_rtt);
	}
	min_rtt = min(min_rtt, cur_time - sent_time);

	// update rtt_acked_ewma
	assert(cur_time > sent_time);
	rtt_acked_ewma.update((cur_time - sent_time),
		cur_time / min_rtt);
	rtt_acked_ewma.round();

	// Update the delta-wise statistics tracker
	if (sent_time != prev_ack_sent_time) {
		ExplicitDeltaData& x = delta_data[cur_delta_class];
		if (x.avg_tpt == 0) {
			assert(x.avg_rtt == 0);
			x.avg_tpt = alpha_delta_data / (sent_time - prev_ack_sent_time);
			x.avg_rtt = cur_time - sent_time;
		}
		else {
			x.avg_tpt = alpha_delta_data / (sent_time - prev_ack_sent_time)
				+ (1.0 - alpha_delta_data) * x.avg_tpt;
			x.avg_rtt = alpha_delta_data * (cur_time - sent_time)
				+ (1.0 - alpha_delta_data) * x.avg_rtt;
			sum_rtt += cur_time - sent_time;
		}
	}
	else {
		cout << "Zero intersend time!!" << endl;
	}

	// update intersend_ewma
	if (prev_ack_sent_time != 0.0) {
		intersend_ewma.update(sent_time - prev_ack_sent_time, \
			cur_time / min_rtt);
		// intersend_ewma.round();
		if (intersend_ewma == 0) {
			cout << "Intersend time 0. Making it large " << sent_time << " " << cur_time << " " << min_rtt << endl << flush;
			intersend_ewma.force_set(1, cur_time / min_rtt);
		}
		else if (intersend_ewma > 0.5)
			intersend_ewma.force_set(sent_time - prev_ack_sent_time, cur_time / min_rtt);
		update_intersend_time();
	}
	else {
	  //cout << "First ack to be considered.\n" << endl;
	}
	prev_ack_sent_time = sent_time;

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
	last_sent_seq_num = seq_num;
}

double sqr(double x) { return x*x; }

void MarkovianCC::close() {
	//double cur_time = current_timestamp();
	//auto& x = delta_data[cur_delta_class];
	//cout << "Closing for delta class " << cur_delta_class << " " << cur_time << " " << num_pkts_acked << endl;
	//if (num_pkts_acked == 0)
	//  return;
	//if (x.avg_tpt == 0) {
	//	x.avg_tpt = num_pkts_acked / cur_time;
	// 	x.var_tpt = 0.0;
	// 	x.avg_rtt = sum_rtt / num_pkts_acked;
	// 	x.var_rtt = 0.0;
	// 	return;
	// }

	// x.avg_tpt = alpha_delta_data * num_pkts_acked / cur_time +
	// 	(1.0 - alpha_delta_data) * x.avg_tpt;
	// x.var_tpt = alpha_delta_data * sqr(num_pkts_acked / cur_time - x.avg_tpt) +
	// 	(1.0 - alpha_delta_data) * x.var_tpt;
	// x.avg_rtt = alpha_delta_data * sum_rtt / num_pkts_acked +
	// 	(1.0 - alpha_delta_data) * x.avg_rtt;
	// x.var_rtt = alpha_delta_data * sqr(sum_rtt / num_pkts_acked - x.avg_rtt) +
	// 	(1.0 - alpha_delta_data) * x.var_rtt;

}

void MarkovianCC::interpret_config_str(string config) {
	// Format - delta_update_type:param1:param2...
	// Delta update types:
	//   -- constant_delta - params:- delta value
	//   -- pfabric_fct - params:- none
	//   -- bounded_delay - params:- delay bound (s)
	delta = 1.0; // If delta is not set in time, we don't want it to be 0
	if (config.substr(0, 15) == "constant_delta:") {
		utility_mode = CONSTANT_DELTA;
		delta = atof(config.substr(15, string::npos).c_str());
		cout << "Constant delta mode with delta = " << delta << endl;
	}
	else if (config.substr(0, 11) == "pfabric_fct") {
		utility_mode = PFABRIC_FCT;
		cout << "Minimizing FCT PFabric style" << endl;
	}
	else if (config.substr(0, 14) == "bounded_delay:") {
		utility_mode = BOUNDED_DELAY;
		delay_bound = stof(config.substr(14, string::npos).c_str());
		cout << "Bounding delay to " << delay_bound << "s" << endl;
	}
	else {
		utility_mode = CONSTANT_DELTA;
		delta = 1.0;
		cout << "Incorrect format of configuration string '" << config
				 << "'. Using constant delta mode with delta = 1 by default" << endl;
	}
}
