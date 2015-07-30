#ifndef NASHCC_HH
#define NASHCC_HH 

#include <chrono>
#include <limits>
#include <map>

#include "ccc.hh"
#include "exponential.hh"

class NashCC : public CCC { 
public:
	enum UtilityMode {CONSTANT_DELTA, MAX_DELAY};

private:
	union NashCCParams {
		struct {
			double delta;
		} constant_delta;
		struct {
			const double alpha_rtt = 1.0/256.0;
			double queueing_delay_limit; // in ms
		} max_delay;
	} params;

	UtilityMode mode;

	double delta;
	const double alpha_rtt = 1.0/16.0;
	const double alpha_intersend = 1.0/16.0;

	// set of all unacked pkts Format: (seq_num, sent_timestamp)
	//
	// Note: a packet is assumed to be lost if a packet with a higher
	// sequence number is acked. This set contains only the packets 
	// which are NOT lost
	std::map<int, double> unacknowledged_packets;
	// Format: (seq_num, delta)
	//
	// The delta corresponds to the delta in the CC protocol when that
	// packet was transmitted
	std::map<int, double> delta_history;
	
	double min_rtt;
	// the rtts are really the queueing delay (ie. measured_rtt - min_rtt)
	double rtt_acked_ewma;  // estimated using only acked packets
	double rtt_unacked_ewma; // unacked packets are also considered
	double intersend_ewma;
	// send time of previous ack. Used to calculate intersend time
	double prev_ack_sent_time;

	// last timepoint at which the various ewmas were updated
	double rtt_acked_ewma_last_update;
	double intersend_ewma_last_update;

	PRNG prng;
	std::chrono::high_resolution_clock::time_point start_time_point;

	// a pkt is considered lost if a gap appears in acks
	unsigned int num_pkts_lost;
	// to calculate % lost pkts
	unsigned int num_pkts_acked;

	// rounds double values to minimize floating pt. effects
	static void round(double& val);
	
	// return a timestamp in milliseconds
	double current_timestamp();

	// update intersend time based on rtt ewma and intersend ewma
	void update_intersend_time();

public:
	NashCC( UtilityMode s_mode, double param1 ) 
	: 	CCC(), 
		params{},
		mode(),
		delta(),
		unacknowledged_packets(),
		delta_history(),
		min_rtt(),
		rtt_acked_ewma(),
		rtt_unacked_ewma(),
		intersend_ewma(),
		prev_ack_sent_time(),
		rtt_acked_ewma_last_update(),
		intersend_ewma_last_update(),
		prng(current_timestamp()),
		start_time_point(),
		num_pkts_lost(),
		num_pkts_acked()
	{
		mode = s_mode;
		if (mode == UtilityMode::CONSTANT_DELTA) {
			params.constant_delta.delta = param1;
			delta = param1;
		}
		else if (mode == UtilityMode::MAX_DELAY) {
			params.max_delay.queueing_delay_limit = param1;
			delta = 1.0; // will evolve with time
		}
	}

	// callback functions for packet events
	virtual void init() override;
	virtual void onACK(int ack, double receiver_timestamp) override ;
	virtual void onPktSent(int seq_num) override ;
	virtual void onTimeout() override { std::cerr << "Ack timed out!\n"; }
	virtual void onLinkRateMeasurement( double s_measured_link_rate ) override;
};

#endif