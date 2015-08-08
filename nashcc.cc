#include "nashcc.hh"

#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

double NashCC::current_timestamp( void ){
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
	delta_history.clear();

	// check note in onPktSent before changing
	min_rtt = numeric_limits<double>::max(); 

	rtt_acked_ewma.reset();
	rtt_unacked_ewma.reset();
	intersend_ewma.reset();
 	_intersend_time = 10;
	prev_ack_sent_time = 0.0;

	last_rtt = max_rtt = 0.0;
	while (!rtt_history.empty())
		rtt_history.pop();

	_the_window = numeric_limits<int>::max();//10;
	_timeout = 1000;

	num_pkts_lost = num_pkts_acked = 0;
	
	start_time_point = std::chrono::high_resolution_clock::now();
}

void NashCC::update_intersend_time(double cur_time) {
	if (!intersend_ewma.is_valid())
	 	return;//_intersend_time = (delta + 1) / (1.0/rtt_ewma);
	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);
	if (rtt_ewma == 0.0)
		return; // can happen for first few packets as min_rtt estimate is not precise

	// use predicted instead of measured rtt
	/*unsigned int tmp_cur_id = (rtt_ewma / (max_rtt - min_rtt))\
			* num_markov_chain_states;
	tmp_cur_id = min(tmp_cur_id, num_markov_chain_states-1); // in case it is called from onPktSent and rtt_unacked_ewma > rtt_acked_ewma
	double tmp_rtt_prediction =  0.5 * (max_rtt - min_rtt) \
				/ num_markov_chain_states;
	unsigned int tmp_i __attribute((unused))= 0;
	for (unsigned int i = 0;i < num_markov_chain_states; i++) {
		tmp_rtt_prediction += markov_chain[tmp_cur_id][i]*(i + 0.5) 
			* (max_rtt - min_rtt) / num_markov_chain_states;
	}
	rtt_ewma = tmp_rtt_prediction;*/

	double tmp = _intersend_time;
	_intersend_time = (delta + 1) / (1.0/rtt_ewma + 1.0/intersend_ewma);
	// _intersend_time = delta * rtt_ewma;
	// cout << _intersend_time << " " << rtt_ewma << endl;
	if (num_pkts_acked < 10)
	 	_intersend_time = max(_intersend_time, tmp); // to avoid bursts due tp min_rtt updates
	if (num_pkts_acked > 10 && num_pkts_acked < 20) { // fast start
		_intersend_time = max(delta, 1.0) * rtt_ewma;
		// cerr << _intersend_time << endl;
		intersend_ewma.reset();
		intersend_ewma.update(_intersend_time, cur_time / min_rtt);
	}
	// if (num_pkts_acked % 1000 == 0)
	// 	cout << _intersend_time << " " << rtt_ewma << endl;
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
			// delta -= 0.1;
			last_delta_update -= 0.1;
		}
		else {
			// delta += 0.1;
			last_delta_update += 0.1;
		}
		delta += last_delta_update;

		//delta = 0.9*delta + 0.1*average_delta;
		delta = max(0.01, delta);

		num_rtt_measurements = 0;
		average_rtt = 0.0;
		// cout << delta << endl;
	}
}

void NashCC::onACK(int ack, double receiver_timestamp __attribute((unused))) {
	int seq_num = ack - 1;
	
	// some error checking
	if ( unacknowledged_packets.count( seq_num ) > 1 ) { 
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) { 
		std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

		double sent_time = unacknowledged_packets[seq_num];
	double cur_time = current_timestamp();

	// before updaing min and max rtt, reset markov chain if either changes
	if (min_rtt > cur_time - sent_time || max_rtt < cur_time - sent_time) {
		//if (cur_time < 10000.0) {
			for (unsigned int i = 0;i < num_markov_chain_states; i++) {
				for (auto & x : markov_chain[i])
					x = 0.0;
				markov_chain[i][i] = 1.0;
			}
		//}
	}

	// update min and max rtt
	if (cur_time - sent_time < min_rtt){
		if (min_rtt != numeric_limits<double>::max()) {
			rtt_acked_ewma.add( min_rtt - (cur_time - sent_time) );
			rtt_unacked_ewma.add( min_rtt - (cur_time - sent_time) );
		}
		min_rtt = cur_time - sent_time;
	}
	max_rtt = max(max_rtt, cur_time - sent_time);

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

	// manage rtt_history
	while (!rtt_history.empty() && rtt_history.front().first \
		< sent_time - max(rtt_acked_ewma, rtt_unacked_ewma) - min_rtt)
		rtt_history.pop();
	rtt_history.push(make_pair(cur_time, cur_time - sent_time - min_rtt));
	double tmp_last_rtt = rtt_history.front().second;

	// update markov chain's transition matrix
	if (num_pkts_acked > 10) { // ideally this should be 10 pkts after last change in min_rtt
		unsigned int tmp_last_id = min((unsigned int)((tmp_last_rtt / (max_rtt - min_rtt)) \
			* num_markov_chain_states), num_markov_chain_states-1);
		unsigned int tmp_cur_id = min((unsigned int)(((cur_time - sent_time - min_rtt) \
			/ (max_rtt - min_rtt)) * num_markov_chain_states), \
			num_markov_chain_states-1);
		for (auto & x : markov_chain[tmp_last_id]){
			x *= (1.0 - alpha_markov_chain);
		}
		markov_chain[tmp_last_id][tmp_cur_id] += alpha_markov_chain * 1;
		// cout << "Changing " << tmp_last_id << " to " << tmp_cur_id << " " << tmp_last_rtt << endl;
	}
	last_rtt = cur_time - sent_time - min_rtt;

	// adjust delta
	//cout << cur_time - sent_time - min_rtt << " " << min_rtt << " " << intersend_ewma << endl;
	if (mode == UtilityMode::MAX_DELAY) {
		delta_update_max_delay(cur_time - sent_time, cur_time);
	}

	// print markov chain's transition matrix
	// static int last_tnsmit = 0;
	// if (int(cur_time/1000.0) % 10 == 0 && last_tnsmit != int(cur_time/1000.0)) {
	// 	for (auto & x : markov_chain){
	// 		double max_val = x[0];
	// 		for (auto & y : x)
	// 			max_val = max(max_val, y);
	// 		for (auto & y : x)
	// 			cout << ((y == max_val)?y:0) << " ";
	// 		cout << endl;
	// 	}
	// 	cout << min_rtt << " " << max_rtt << endl;
	// 	last_tnsmit = int(cur_time/1000.0);
	// }

	// delete this pkt and any unacknowledged pkts before this pkt
	for (auto x : unacknowledged_packets) {
		if(x.first > seq_num)
			break;
		if(x.first < seq_num) {
			++ num_pkts_lost;
			//cout << "Lost: " << seq_num << " " << x.first << " " << cur_time - sent_time << " " << _intersend_time << endl;
		}
		unacknowledged_packets.erase(x.first);
		delta_history.erase(x.first);
	}
	++ num_pkts_acked;

	if (_the_window < numeric_limits<int>::max())
		_the_window = numeric_limits<int>::max();

	// cout << "Got: \t\t\t\t\t" << cur_time << " " << rtt_acked_ewma << " " << _intersend_time << " " << cur_time - sent_time << " " << min_rtt << endl << flush;
}

void NashCC::onLinkRateMeasurement( double s_measured_link_rate __attribute((unused)) ) {
	assert( false );
}

void NashCC::onPktSent(int seq_num) {
	// add to list of unacknowledged packets
	assert( unacknowledged_packets.count( seq_num ) == 0 );
	double cur_time = current_timestamp();
	unacknowledged_packets[seq_num] = cur_time;

	// add to delta_history
	assert( delta_history.count( seq_num ) == 0 );
	delta_history[seq_num] = delta;

	// check if rtt_ewma is to be increased
	rtt_unacked_ewma = rtt_acked_ewma;
	for (auto & x : unacknowledged_packets) {
		double rtt_lower_bound = cur_time - min_rtt - x.second;
		if (min_rtt == numeric_limits<double>::max())
			rtt_lower_bound = cur_time - x.second;
		if (rtt_lower_bound <= rtt_unacked_ewma)
			break;
		rtt_unacked_ewma.update(rtt_lower_bound, cur_time);
		cout << rtt_lower_bound << " " << rtt_unacked_ewma << " " << rtt_acked_ewma << endl;
	}
	rtt_unacked_ewma.round();
	/*double rtt_lower_bound = cur_time - min_rtt \
		- unacknowledged_packets.begin()->second;
	if (min_rtt == numeric_limits<double>::max())
		rtt_lower_bound = cur_time - unacknowledged_packets.begin()->second;

	rtt_unacked_ewma = rtt_acked_ewma;
	if (rtt_lower_bound > rtt_acked_ewma) {
		rtt_unacked_ewma.update(rtt_lower_bound, cur_time);
		rtt_unacked_ewma.round();

		update_intersend_time(cur_time);
	}*/

	// cout << "Sent: " << cur_time << " " << rtt_acked_ewma << " " << _intersend_time << endl;
}