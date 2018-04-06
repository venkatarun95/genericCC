#ifndef MARKOVIANCC_HH
#define MARKOVIANCC_HH

#undef SIMULATION_MODE

#include "ccc.hh"
#ifdef SIMULATION_MODE
#include "random.h"
#else
#include "random.hh"
#endif
#include "estimators.hh"
#include "rtt-window.hh"

#include <chrono>
#include <functional>
#include <iostream>
#include <map>

class MarkovianCC : public CCC {
  typedef double Time;
  typedef int SeqNum;
  
  double delta;
  
  // Some adjustable parameters
  static constexpr double alpha_rtt = 1.0 / 1.0;
  // This factor is normalizes w.r.t expected Newreno window size for
  // TCP cooperation
  static constexpr double alpha_loss = 1.0 / 2.0;
  static constexpr double alpha_rtt_long_avg = 1.0 / 4.0;
  static constexpr double rtt_averaging_interval = 0.1;
  static constexpr int num_probe_pkts = 10;
  static constexpr double copa_k = 2;
  
  struct PacketData {
    Time sent_time;
    Time intersend_time;
    Time intersend_time_vel;
    Time rtt;
    double prev_avg_sending_rate;
  };
  
  std::map<SeqNum, PacketData> unacknowledged_packets;
  
  Time min_rtt;
  // If min rtt is supplied externally, preserve across flows.
  double external_min_rtt;
  PlainEwma rtt_unacked;
  RTTWindow rtt_window;
  Time prev_intersend_time;
  Time cur_intersend_time;
  Percentile interarrival;
  IsUniformDistr is_uniform;

  Time last_update_time;
  int update_dir;
  int prev_update_dir;
  int pkts_per_rtt;
  double update_amt;
  
  Time intersend_time_vel;
  Time prev_intersend_time_vel;
  double prev_rtt; // Measured RTT one-rtt ago.
  double prev_rtt_update_time;
  double prev_avg_sending_rate;
  
#ifdef SIMULATION_MODE
  RNG rand_gen;
#else
  RandGen rand_gen;
#endif
  int num_pkts_lost;
  int num_pkts_acked;
  int num_pkts_sent;
  
  // Variables for expressing explicit utility
  enum {CONSTANT_DELTA, PFABRIC_FCT, DEL_FCT, BOUNDED_DELAY, BOUNDED_DELAY_END,
	MAX_THROUGHPUT, BOUNDED_QDELAY_END, BOUNDED_FDELAY_END, TCP_COOP,
        CONST_BEHAVIOR, AUTO_MODE} utility_mode;
  enum {DEFAULT_MODE, LOSS_SENSITIVE_MODE, TCP_MODE} operation_mode;
  bool do_slow_start;
  bool keep_ext_min_rtt;
  double default_delta;
  int flow_length;
  double delay_bound;
  double prev_delta_update_time;
  double prev_delta_update_time_loss;
  double max_queuing_delay_estimate;
  // To cooperate with TCP, measured in fraction of RTTs with loss
  LossRateEstimate loss_rate;
  ReduceOnLoss reduce_on_loss;
  bool loss_in_last_rtt;
  // Behavior constant
  double behavior;
  PlainEwma interarrival_ewma;
  double prev_ack_time;
  double exp_increment;
  PlainEwma prev_delta;
  bool slow_start;
  double slow_start_threshold;
  
  // Find flow id
  static int flow_id_counter;
  int flow_id;
  
  Time cur_tick;
  
  double current_timestamp();
  
  double randomize_intersend(double);
  
  void update_intersend_time();
  
  void update_delta(bool pkt_lost, double cur_rtt=0);
  
public:
  void interpret_config_str(std::string config);
  
  MarkovianCC(double delta)
    : delta(delta),
      unacknowledged_packets(),
      min_rtt(),
      external_min_rtt(false),
      rtt_unacked(alpha_rtt),
      rtt_window(),
      prev_intersend_time(),
      cur_intersend_time(),
      interarrival(0.05),
      is_uniform(32),
      last_update_time(),
      update_dir(),
      prev_update_dir(),
      pkts_per_rtt(),
      update_amt(),
      intersend_time_vel(),
      prev_intersend_time_vel(),
      prev_rtt(),
      prev_rtt_update_time(),
      prev_avg_sending_rate(),
      rand_gen(),
      num_pkts_lost(),
      num_pkts_acked(),
      num_pkts_sent(),
      utility_mode(CONSTANT_DELTA),
      operation_mode(DEFAULT_MODE),
      do_slow_start(false),
      keep_ext_min_rtt(false),
      default_delta(0.5),
      flow_length(),
      delay_bound(),
      prev_delta_update_time(),
      prev_delta_update_time_loss(),
      max_queuing_delay_estimate(),
      loss_rate(),
      reduce_on_loss(),
      loss_in_last_rtt(),
      behavior(),
      interarrival_ewma(1.0 / 32.0),
      prev_ack_time(),
      exp_increment(),
      prev_delta(1.0),
      slow_start(),
      slow_start_threshold(),
      flow_id(++ flow_id_counter),
      cur_tick()
  {}
  
  // callback functions for packet events
  virtual void init() override;
  virtual void onACK(int ack, double receiver_timestamp, 
		     double sent_time, int delta_class=-1);
  virtual void onTimeout() override;
  virtual void onDupACK() override;
  virtual void onPktSent(int seq_num) override;
  void onTinyPktSent() {num_pkts_acked ++;}
  virtual void close() override;
  
  bool send_tiny_pkt() {return false;}//num_pkts_acked < num_probe_pkts-1;}
  
  //#ifdef SIMULATION_MODE
  void set_timestamp(double s_cur_tick) {cur_tick = s_cur_tick;}
  //#endif
  
  void set_flow_length(int s_flow_length) {flow_length = s_flow_length;}
  void set_min_rtt(double x) {
    if (external_min_rtt == 0)
      external_min_rtt = x;
    else
      external_min_rtt = std::min(external_min_rtt, x);
    init();
    std::cout << "Set min. RTT to " << external_min_rtt << std::endl;
  }
  int get_delta_class() const {return 0;}
  double get_the_window() {return _the_window;}
  void set_delta_from_router(double x) {
    if (utility_mode == BOUNDED_DELAY) {
      if (delta != x)
        std::cout << "Delta changed to: " << x << std::endl;
      delta = x;
    }
  }
};

#endif
