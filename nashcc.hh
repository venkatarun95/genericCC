#ifndef NASHCC_HH
#define NASHCC_HH 

#include <chrono>
#include <limits>
#include <map>

#include "ccc.hh"
#include "exponential.hh"

class NashCC : public CCC { 
private:
	double delta = 1;
	const double alpha_rtt = 1.0/16.0;
	const double alpha_intersend = 1.0/16.0;

	// set of all unacked pkts Format: (seq_num, sent_timestamp)
	//
	// Note: a packet is assumed to be lost if a packet with a higher
	// sequence number is acked. This set contains only the packets 
	// which are NOT lost
	std::map<int, double> unacknowledged_packets;
	
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
	
	// return a timestamp in milliseconds
	double current_timestamp();

	// update intersend time based on rtt ewma and intersend ewma
	void update_intersend_time();

public:
	NashCC( double s_delta ) 
	: 	CCC(), 
		delta( s_delta ),
		unacknowledged_packets(),
		min_rtt(),
		rtt_acked_ewma(),
		rtt_unacked_ewma(),
		intersend_ewma(),
		prev_ack_sent_time(),
		rtt_acked_ewma_last_update(),
		intersend_ewma_last_update(),
		prng(current_timestamp()),
		start_time_point()
	{}

	// callback functions for packet events
	virtual void init() override;
	virtual void onACK(int ack, double receiver_timestamp) override ;
	virtual void onPktSent(int seq_num) override ;
	virtual void onTimeout() override { std::cerr << "Ack timed out!\n"; }
	virtual void onLinkRateMeasurement( double s_measured_link_rate ) override;
};

#endif