#include "nashcc.hh"

#include <cassert>
#include <cmath>
#include <limits>

using namespace std;

void NashCC::round(double& val){
	val = int(val*100) / 100.0;
}

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

	// check note in onPktSent before changing
	min_rtt = numeric_limits<double>::max(); 
	num_pkts_since_last_min_rtt_update = 0;
	rtt_acked_ewma = rtt_unacked_ewma = intersend_ewma = 0.0;
	rtt_acked_ewma_last_update = intersend_ewma_last_update = 0.0;

	_intersend_time = 20;
	prev_ack_sent_time = 0.0;

	_the_window = numeric_limits<int>::max();//10;
	_timeout = 1000;

	num_pkts_lost = num_pkts_acked = 0;
	
	start_time_point = std::chrono::high_resolution_clock::now();
}

void NashCC::update_intersend_time() {
	if (intersend_ewma_last_update == 0.0)
	 	return;//_intersend_time = (delta + 1) / (1.0/rtt_ewma);
	double rtt_ewma = max(rtt_unacked_ewma, rtt_acked_ewma);
	if (rtt_ewma == 0.0)
		return; // can happen for first few packets as min_rtt estimate is not precise

	double tmp = _intersend_time;
	_intersend_time = (delta + 1) / (1.0/rtt_ewma + 1.0/intersend_ewma);
	if (num_pkts_since_last_min_rtt_update < 10)
	 	_intersend_time = max(_intersend_time, tmp); // to avoid bursts due tp min_rtt updates
	round(intersend_ewma);
	//cout << "Update: " << rtt_ewma << " " << intersend_ewma << endl;
	
	//Exponential dist(_intersend_time, prng);
	//_intersend_time = dist.sample();
}

void NashCC::onACK(int ack, double receiver_timestamp __attribute((unused))) {
	int seq_num = ack - 1;
	
	// some error checking
	if ( unacknowledged_packets.count( seq_num ) > 1 ) { 
		std::cerr<<"Dupack: "<<seq_num<<std::endl; return; }
	if ( unacknowledged_packets.count( seq_num ) < 1 ) { 
		std::cerr<<"Unknown Ack!! "<<seq_num<<std::endl; return; }

	// update rtt_acked_ewma
	double sent_time = unacknowledged_packets[seq_num];
	double cur_time = current_timestamp();
	if (cur_time - sent_time < min_rtt){
		if (min_rtt != numeric_limits<double>::max()) {
			rtt_acked_ewma += min_rtt - (cur_time - sent_time);
			rtt_unacked_ewma += min_rtt - (cur_time - sent_time);
		}
		min_rtt = cur_time - sent_time;
		//num_pkts_since_last_min_rtt_update = 0;
	}
	++ num_pkts_since_last_min_rtt_update;

	double ewma_factor = 1;
	if (rtt_acked_ewma != 0.0) {
		ewma_factor = pow(alpha_rtt, 
			(cur_time - rtt_acked_ewma_last_update) / min_rtt);
	}
	if (cur_time - sent_time - min_rtt == 0) {
		ewma_factor = 0.0;
	}
	rtt_acked_ewma = ewma_factor * (cur_time - sent_time - min_rtt) + \
		(1 - ewma_factor) * rtt_acked_ewma;
	rtt_acked_ewma_last_update = cur_time;
	round(rtt_acked_ewma);

	// update intersend_ewma
	if (intersend_ewma_last_update != 0.0) { 
		ewma_factor = pow(alpha_intersend, \
			(cur_time - intersend_ewma_last_update) / min_rtt);
		intersend_ewma = ewma_factor * (sent_time - prev_ack_sent_time) \
			+ (1 - ewma_factor) * intersend_ewma;
		update_intersend_time();
	}
	intersend_ewma_last_update = cur_time;
	prev_ack_sent_time = sent_time;

	// delete this pkt and any unacknowledged pkts before this pkt
	for (auto x : unacknowledged_packets) {
		if(x.first > seq_num)
			break;
		if(x.first < seq_num) {
			++ num_pkts_lost;
			cout << "Lost: " << seq_num << " " << x.first << " " << cur_time - sent_time << " " << _intersend_time << endl;
		}
		unacknowledged_packets.erase(x.first);
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

	// check if rtt_ewma is to be increased
	double rtt_lower_bound = cur_time - min_rtt \
		- unacknowledged_packets.begin()->second;
	if (min_rtt == numeric_limits<double>::max())
		rtt_lower_bound = cur_time - unacknowledged_packets.begin()->second;
	if (rtt_lower_bound > rtt_acked_ewma) {
		double ewma_factor = 1;
		if (rtt_acked_ewma_last_update != 0)
			ewma_factor = pow(alpha_rtt, 
				(cur_time-rtt_acked_ewma_last_update) / min_rtt);

		//if (rtt_acked_ewma != 0)
			rtt_unacked_ewma = ewma_factor * rtt_lower_bound \
				+ (1 - ewma_factor) * rtt_acked_ewma;
		// else 
		// 	rtt_unacked_ewma = ewma_factor * rtt_lower_bound + (1 - ewma_factor) * rtt_unacked_ewma;

		round(rtt_unacked_ewma);

		update_intersend_time();
	}
	else
		rtt_unacked_ewma = rtt_acked_ewma;

	// cout << "Sent: " << cur_time << " " << rtt_acked_ewma << " " << _intersend_time << endl;
}