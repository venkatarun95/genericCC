#ifndef MARKOVIANCC_HH
#define MARKOVIANCC_HH

#define SIMULATION_MODE // Take timestamp from lower layer

#include "ccc.hh"
#include "random.hh"
#include "utilities.hh"

#include <chrono>
#include <functional>
#include <map>

class MarkovianCC : public CCC {
	typedef double Time;
	typedef int SeqNum;

	double delta;

	// Some adjustable parameters
	static constexpr double alpha_rtt = 1.0 / 8.0;
  // This factor is normalizes w.r.t expected Newreno window size for
  // TCP cooperation
	static constexpr double alpha_loss = 1.0 / 2.0;
	static constexpr double rtt_averaging_interval = 0.1;
	static constexpr int num_probe_pkts = 2;
	static constexpr double alpha_interarrival_time = 1.0 / 2.0;
	static constexpr double delta_decay_rate = 0.84; // sqrt(0.5)

	struct PacketData {
		Time sent_time;
		Time intersend_time;
		Time intersend_time_vel;
	};

	std::map<SeqNum, PacketData> unacknowledged_packets;
	
	Time min_rtt;
	TimeEwma rtt_acked;
	TimeEwma rtt_unacked;
	Time prev_intersend_time;
	Time cur_intersend_time;
	
	Time intersend_time_vel;
	Time prev_intersend_time_vel;

	RandGen rand_gen;
	int num_pkts_lost;
	int num_pkts_acked;

	// Variables for dealing with loss
	// We monitor number of losses every RTT
	Time monitor_interval_start;
	// Number of losses in the current RTT
	int num_losses;
	// Number of losses in the previous RTT
	int prev_num_losses;
	PlainEwma interarrival_time;
	// For calculating interarrival_time
	Time prev_ack_time;
	Time pseudo_delay;
	Time prev_pseudo_delay;

	// Variables for expressing explicit utility
	enum {CONSTANT_DELTA, PFABRIC_FCT, DEL_FCT, BOUNDED_DELAY, BOUNDED_DELAY_END,
				BOUNDED_PERCENTILE_DELAY_END, TCP_COOP, MAX_THROUGHPUT, BOUNDED_QDELAY_END,
        BOUNDED_FDELAY_END} utility_mode;
	int flow_length;
	double delay_bound;
	double prev_delta_update_time;
	double prev_delta_update_time_loss;
	Percentile percentile_delay;
  // To cooperate with TCP, measured in fraction of RTTs with loss
	double loss_rate;
  bool loss_in_last_rtt;

	#ifdef SIMULATION_MODE
	Time cur_tick;
	#else
	std::chrono::high_resolution_clock::time_point start_time_point;
  #endif

	double current_timestamp();

	double randomize_intersend(double);

	void update_intersend_time();
	
	void update_delta(bool pkt_lost);
	
public:
	void interpret_config_str(std::string config);
	
	MarkovianCC(double delta)
	: delta(delta),
		unacknowledged_packets(),
		min_rtt(),
		rtt_acked(alpha_rtt),
		rtt_unacked(alpha_rtt),
		prev_intersend_time(),
		cur_intersend_time(),
		intersend_time_vel(),
		prev_intersend_time_vel(),
		rand_gen(),
	  num_pkts_lost(),
		num_pkts_acked(),
		monitor_interval_start(),
		num_losses(),
	  prev_num_losses(),
		interarrival_time(alpha_interarrival_time),
		prev_ack_time(),
		pseudo_delay(),
		prev_pseudo_delay(),
		utility_mode(CONSTANT_DELTA),
		flow_length(),
		delay_bound(),
		prev_delta_update_time(),
		prev_delta_update_time_loss(),
		percentile_delay(),
    loss_rate(),
    loss_in_last_rtt(false),
		#ifdef SIMULATION_MODE
		cur_tick()
    #else
		start_time_point()
		#endif
	{}

	// callback functions for packet events
	virtual void init() override;
	virtual void onACK(int ack, double receiver_timestamp, 
										 double sent_time) override;
	virtual void onPktSent(int seq_num) override;
	void onTinyPktSent() {num_pkts_acked ++;}
	virtual void close() override;

	bool send_tiny_pkt() {return num_pkts_acked < num_probe_pkts-1;}

  #ifdef SIMULATION_MODE
	void set_timestamp(double s_cur_tick) {cur_tick = s_cur_tick;}
	#endif

	void set_flow_length(int s_flow_length) {flow_length = s_flow_length;}
	void set_min_rtt(double x) {min_rtt = x; std::cout << "Set min. RTT to " << x << std::endl;}
	int get_delta_class() {return 0;}

	void log_data(double cur_time __attribute((unused))) {
	  //cout << cur_time << " " << _intersend_time << " " << rtt_acked << " " << rtt_unacked << " " << min_rtt << endl;
  }
};

#endif
