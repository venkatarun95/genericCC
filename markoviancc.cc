#undef NDEBUG // We want the assert statements to work

#include "markoviancc.hh"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
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
		cout << "% Packets Lost: " << (100.0 * num_pkts_lost) /
			(num_pkts_acked + num_pkts_lost) << endl;

	//min_rtt = numeric_limits<double>::max();

	unacknowledged_packets.clear();

	rtt_acked.reset();
	rtt_unacked.reset();
	prev_intersend_time = 0;

	intersend_time_vel = 0;
	prev_intersend_time_vel = 0;

	_intersend_time = 0;
	_the_window = num_probe_pkts;
	_timeout = 1000;
	
	monitor_interval_start = 0.0;
	num_losses = 0;
	prev_num_losses = 0;
	interarrival_time.reset();
	prev_ack_time = 0.0;
	pseudo_delay = 0.0;
	prev_pseudo_delay = 0.0;

	num_pkts_acked = num_pkts_lost = 0;
	flow_length = 0;

	#ifdef SIMULATION_MODE
	cur_tick = 0;
	#else
	start_time_point = std::chrono::high_resolution_clock::now();
  #endif
}

void MarkovianCC::update_delta(bool pkt_lost) {
	double cur_time = current_timestamp();
	double rtt_ewma = max(rtt_acked, rtt_unacked);
	if (utility_mode == BOUNDED_PERCENTILE_DELAY_END)
		rtt_ewma = percentile_delay.get_percentile_value();

	if (utility_mode == PFABRIC_FCT) {
		delta = 0.05 + 1.0 * (1.0 - exp(-1.0 * 0.00001 * flow_length));
  }
	else if (utility_mode == BOUNDED_DELAY_END 
					 || utility_mode == BOUNDED_PERCENTILE_DELAY_END) {
		if (prev_delta_update_time_loss < cur_time - rtt_ewma && pkt_lost) {
			delta += 0.01;
			//delta *= 1.1;
			//cout << "@ " << cur_time << " " << delta << " Loss" << endl;
			prev_delta_update_time_loss = cur_time;
			return;
		}

		if (prev_delta_update_time > cur_time - rtt_ewma)
			return;
		prev_delta_update_time = cur_time;
		
		if (rtt_ewma > delay_bound)
			delta += 0.01; //*= 1.1;
		else if (rtt_ewma < delay_bound)
			delta /= 1.1; //1.0 / (1.0 / delta + 0.01);
		delta = max(0.01, delta);
		//cout << "@ " << cur_time << " " << delta << " " << rtt_ewma << " " << delay_bound << " " << pkt_lost << endl;
	}
	assert(utility_mode == PFABRIC_FCT || utility_mode == CONSTANT_DELTA  ||
				 utility_mode == BOUNDED_DELAY_END || 
				 utility_mode == BOUNDED_PERCENTILE_DELAY_END);
}

double MarkovianCC::randomize_intersend(double intersend) {
	if (intersend == 0)
		return 0;
	return rand_gen.exponential(intersend);
	//return rand_gen.uniform(0.5*intersend, 1.5*intersend);
	//return intersend;
}

void MarkovianCC::update_intersend_time() { 
	double cur_time __attribute((unused)) = current_timestamp();
	if (num_pkts_acked < num_probe_pkts - 1)
		return;
	_the_window = numeric_limits<double>::max();
	constexpr double link_intersend_time __attribute((unused)) = 9.68e-5;
	
	double queuing_delay = max((double)rtt_acked, (double)rtt_unacked) - min_rtt;
	//queuing_delay += pseudo_delay;
	double target_intersend_time = delta * queuing_delay;

	if (prev_intersend_time != 0)	
		cur_intersend_time = max(0.5 * prev_intersend_time, target_intersend_time);
	else
		cur_intersend_time = target_intersend_time;
	//cur_intersend_time = min(cur_intersend_time, min_rtt / 2.0);

	//if (num_losses >= 3 || prev_num_losses >= 3)
	//cur_intersend_time = max(cur_intersend_time, (double)interarrival_time);

	_intersend_time = randomize_intersend(cur_intersend_time);
	//cout << "@ " << cur_intersend_time << " " << queuing_delay << " " << _intersend_time << " " << rtt_acked << " " << rtt_unacked << endl;
}

void MarkovianCC::onACK(int ack, 
												double receiver_timestamp __attribute((unused)),
												double sent_time) {
	int seq_num = ack - 1;
	double cur_time = current_timestamp();

	assert(cur_time > sent_time);

	rtt_acked.update(cur_time - sent_time, cur_time / min_rtt);
	min_rtt = min(min_rtt, cur_time - sent_time);
	if (rtt_acked < min_rtt) 
		cout << "Warning: RTT < min_rtt" << endl;

	// Update interarrival time
	if (prev_ack_time != 0.0)
		interarrival_time.update(cur_time - prev_ack_time, cur_time / min_rtt);
	pseudo_delay *= pow(delta_decay_rate, (cur_time - prev_ack_time) / min_rtt);
	prev_ack_time = cur_time;
	if (cur_time - monitor_interval_start > max(rtt_acked, rtt_unacked)) {
		monitor_interval_start = cur_time;
		// if (num_losses >= 3)
		// 	cout << "High loss RTT" << endl;
		// else
		// 	cout << "Low loss RTT" << endl;
		//	cout << "Low loss RTT" << endl;
		if (prev_pseudo_delay != 0)
			prev_pseudo_delay = pseudo_delay;
		else
			prev_pseudo_delay = rtt_acked - min_rtt;
		prev_num_losses = num_losses;
		num_losses = 0;
	}

	percentile_delay.push(cur_time - sent_time);
	update_delta(false);
	update_intersend_time();

	if (unacknowledged_packets.count(seq_num) != 0 &&
			unacknowledged_packets[seq_num].sent_time == sent_time) {
		
		int tmp_seq_num = -1;
		for (auto x : unacknowledged_packets) {		
			assert(tmp_seq_num <= x.first);
			tmp_seq_num = x.first;
			if (x.first > seq_num)
				break;
			prev_intersend_time = x.second.intersend_time;
			prev_intersend_time_vel = x.second.intersend_time_vel;
			if (x.first < seq_num) {
				update_delta(true);
				++ num_losses;
				++ num_pkts_lost;
				pseudo_delay += interarrival_time;
				if (prev_pseudo_delay != 0.0)
					pseudo_delay = min(pseudo_delay, 2 * prev_pseudo_delay);
			}
			unacknowledged_packets.erase(x.first);
		}
	}

	++ num_pkts_acked;
}

void MarkovianCC::onPktSent(int seq_num) {
	double cur_time = current_timestamp();
	unacknowledged_packets[seq_num] = {cur_time, cur_intersend_time, intersend_time_vel};

	rtt_unacked.force_set(rtt_acked, cur_time / min_rtt);
	for (auto & x : unacknowledged_packets) {
		if (cur_time - x.second.sent_time > rtt_unacked) {
			rtt_unacked.update(cur_time - x.second.sent_time, cur_time / min_rtt);
			prev_intersend_time = x.second.intersend_time;
			prev_intersend_time_vel = x.second.intersend_time_vel;
			continue;
		}
		break;
	}
	//cout << "@ " << cur_time << " " << cur_intersend_time << " " << rtt_acked << " " << rtt_unacked << endl;

	_intersend_time = randomize_intersend(cur_intersend_time);
}

void MarkovianCC::close() {
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
	else if (config.substr(0, 18) == "bounded_delay_end:") {
		delta = 1;
		utility_mode = BOUNDED_DELAY_END;
		delay_bound = stof(config.substr(18, string::npos).c_str());
		cout << "Bounding delay to " << delay_bound << " ms in an end-to-end manner" << endl;
	}
	else if (config.substr(0, 29) == "bounded_percentile_delay_end:") {
		delta = 0.3;
		utility_mode = BOUNDED_PERCENTILE_DELAY_END;
		delay_bound = stof(config.substr(29, string::npos).c_str());
		cout << "Bounding percentile delay to " << delay_bound << " ms in an end-to-end manner" << endl;
	}
	else {
		utility_mode = CONSTANT_DELTA;
		delta = 1.0;
		cout << "Incorrect format of configuration string '" << config
				 << "'. Using constant delta mode with delta = 1 by default" << endl;
	}
}
