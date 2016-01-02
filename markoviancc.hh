#ifndef MARKOVIANCC_HH
#define MARKOVIANCC_HH

// to decide the source of timestamp
#undef SIMULATION_MODE

#include <chrono>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "ccc.hh"
// #include "exponential.hh"
//#include "random.h"
#include "utilities.hh"

class MarkovianCC : public CCC {
	double delta;

	// some adjustable parameters
	static constexpr double alpha_rtt = 1.0 / 8.0; //1.0/2.0;
	static constexpr double alpha_intersend = 1.0; //1.0/2.0;
	static constexpr double alpha_update = 1.0; //1.0/2.0;
	static constexpr double initial_intersend_time = 0.01;
	static constexpr int num_probe_pkts = 4;
	// set of all unacked pkts Format: (seq_num, sent_timestamp)
	//
	// Note: a packet is assumed to be lost if a packet with a higher
	// sequence number is acked. This set contains only the packets
	// which are NOT lost
	std::map<int, double, std::function<bool(const int&, const int&)> > unacknowledged_packets;

	double min_rtt;

	PlainEwma mean_sending_rate;

	PlainEwma rtt_acked_ewma;  // estimated using only acked packets
	PlainEwma rtt_unacked_ewma; // unacked packets are also considered
	PlainEwma intersend_ewma;
	// send time of previous ack. Used to calculate intersend time
	double prev_ack_sent_time;

	// cur_tick is measured relative to this
	std::chrono::high_resolution_clock::time_point start_time_point;

	// a pkt is considered lost if a gap appears in acks
	unsigned int num_pkts_lost;
	// to calculate % lost pkts
	unsigned int num_pkts_acked;

	//RNG rand_gen;

	// Variables for expressing explicit utility functions

	enum {CONSTANT_DELTA, PFABRIC_FCT, DEL_FCT, BOUNDED_DELAY} utility_mode;
	// When utility_mode == BOUNDED_DELAY, the delay bound in sec
	double delay_bound;

	const std::vector<double> delta_classes = {1, 2, 3, 4, 6, 8, 16, 32};
	// Data stored against each delta value
	struct ExplicitDeltaData {
		double avg_tpt;
		double var_tpt;
		double avg_rtt;
		double var_rtt;

		ExplicitDeltaData()
		:	avg_tpt(0.0),
			var_tpt(0.0),
			avg_rtt(0.0),
			var_rtt(0.0)
		{}
	};
	static constexpr double alpha_delta_data = 1.0 / 128.0;
	std::vector<ExplicitDeltaData> delta_data;
	// To calculate avg. RTT w.r.t num_pkts_acked
	double sum_rtt;

	int cur_delta_class;
	// Last time the delta was updated
	double last_delta_update_time;
	// To make sure we modify sending rate only according to data from
	// the latest delta.
	int last_delta_update_seq_num;
	// Sequence number of the last packet sent. Used to set
	// last_delta_update_seq_num.
	int last_sent_seq_num;
	// If known, length of remaining flow in pkts, otherwise 0.
	int flow_length;

	#ifdef SIMULATION_MODE
	// current time to be used during simulation
	double cur_tick;
	#endif

	// Return a timestamp in milliseconds from previous call of init()
	double current_timestamp();

	// update intersend time based on rtt ewma and intersend ewma
	void update_intersend_time();

	// Update the delta to express explicit utility functions
	void update_delta();

public:
	// Configure the congestion controller according to user options
	void interpret_config_str(std::string config);

	MarkovianCC( double s_delta )
	: 	CCC(),
		delta( s_delta ),
		unacknowledged_packets([](const int& x, const int& y){return x > y;}),
		min_rtt(),
		mean_sending_rate(alpha_update),
		rtt_acked_ewma(alpha_rtt),
		rtt_unacked_ewma(alpha_rtt),
		intersend_ewma(alpha_intersend),
		prev_ack_sent_time(),
		start_time_point(),
		num_pkts_lost(),
		num_pkts_acked(),
		utility_mode(CONSTANT_DELTA),
		delay_bound(0),
		delta_data(delta_classes.size()),
		sum_rtt(0),
		cur_delta_class(0),
		last_delta_update_time(0.0),
		last_delta_update_seq_num(0),
		last_sent_seq_num(0),
		flow_length(0)
		#ifdef SIMULATION_MODE
		,cur_tick()
		#endif
	{}

	// callback functions for packet events
	virtual void init() override;
	virtual void onACK(int ack, double receiver_timestamp, double sent_time) override ;
	virtual void onPktSent(int seq_num) override ;
	virtual void onTimeout() override {}
	virtual void onLinkRateMeasurement( double s_measured_link_rate ) override;
	virtual void close() override;

	#ifdef SIMULATION_MODE
	void set_timestamp(double s_cur_tick) {cur_tick = s_cur_tick;}
	#endif

	// So that it can choose an appropriate delta for min. FCT. Should
	// be specified in num pkts. GenericTCP constantly updates remaining
	// flow length.
	void set_flow_length(int s_flow_length) { flow_length = s_flow_length; }

	int get_delta_class() { return (cur_delta_class >= 0)?cur_delta_class:0; }
	// int get_delta_class() { return (delta == 0.1)?0:1; }
	void set_delta(double s_delta) { delta = s_delta; }
	void log_data(double cur_time) {
		std::cout << cur_time << " " << mean_sending_rate*1040*8 << " " << std::max(rtt_acked_ewma, rtt_unacked_ewma) << " " << _intersend_time<<  std::endl;
	}
};

#endif
