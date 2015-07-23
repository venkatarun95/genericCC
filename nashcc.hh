#ifndef NASHCC_HH
#define NASHCC_HH 

#include <chrono>
#include <limits>
#include <unordered_map>

#include "ccc.hh"
#include "exponential.hh"

class NashCC : public CCC { 
private:
	const double delta = 1;

	std::chrono::high_resolution_clock::time_point start_time_point;
	std::unordered_map<int, double> unacknowledged_packets;
	
	double measured_link_rate;
	double min_rtt;
	double intersend_ewma;

	double current_timestamp();

	PRNG prng;

public:
	NashCC( double s_delta ) 
	: 	CCC(), 
		delta( s_delta ),
		start_time_point(),
		unacknowledged_packets(),
		measured_link_rate(),
		min_rtt(),
		intersend_ewma(),
		prng(current_timestamp())
	{}

	virtual void init() override;
	virtual void onACK(int ack, double receiver_timestamp) override ;
	virtual void onPktSent(int seq_num) override ;
	virtual void onTimeout() override { std::cerr << "Ack timed out!\n"; }
	virtual void onLinkRateMeasurement( double s_measured_link_rate ) override;
};

#endif