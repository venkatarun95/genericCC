#undef NDEBUG // We want the assert statements to work

#include "markoviancc.hh"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

using namespace std;

int MarkovianCC::flow_id_counter = 0;

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
  if (num_pkts_acked != 0) {
    cout << "% Packets Lost: " << (100.0 * num_pkts_lost) /
      (num_pkts_acked + num_pkts_lost) << " at " << current_timestamp() << " " << num_pkts_acked << " " << num_pkts_sent << " min_rtt= " << min_rtt << " " << num_pkts_acked << " " << num_pkts_lost << endl;
    if (slow_start)
      cout << "Finished while in slow-start at window " << _the_window << endl;
  }

  _intersend_time = 0;
  if (external_min_rtt != 0)
    _intersend_time = external_min_rtt / _the_window;
  if (keep_ext_min_rtt)
    min_rtt = external_min_rtt;
  else
    min_rtt = numeric_limits<double>::max();
  _the_window = num_probe_pkts;
  _timeout = 1000;
  
  if (utility_mode != CONSTANT_DELTA)
    delta = 1;
  
  unacknowledged_packets.clear();

  rtt_acked.reset();
  rtt_unacked.reset();
  rtt_window.reset();
  rtt_window.update_window_size(0);
  prev_intersend_time = 0;
  cur_intersend_time = 0;
  interarrival.reset();
  is_uniform.reset();

  
  last_update_time = 0;
  update_dir = 1;
  prev_update_dir = 1;
  pkts_per_rtt = 0;
  update_amt = 1;
  
  intersend_time_vel = 0;
  prev_intersend_time_vel = 0;
  prev_rtt = 0;
  prev_rtt_update_time = 0;
  prev_avg_sending_rate = 0;

  num_pkts_acked = num_pkts_lost = num_pkts_sent = 0;
  operation_mode = DEFAULT_MODE;
  flow_length = 0;
  prev_delta_update_time = 0;
  prev_delta_update_time_loss = 0;
  max_queuing_delay_estimate = 0;
  max_rtt = 0;

  loss_rate.reset();
  loss_in_last_rtt = false;
  rtt_long_avg.reset();
  interarrival_ewma.reset();
  prev_ack_time = 0.0;
  exp_increment = 1.0;
  prev_delta.reset();
  slow_start = true;
  slow_start_threshold = 1e10;
  rtt_var.reset();

#ifdef SIMULATION_MODE
  cur_tick = 0;
#else
  start_time_point = std::chrono::high_resolution_clock::now();
#endif
}

void MarkovianCC::update_delta(bool pkt_lost __attribute((unused)), double cur_rtt __attribute((unused))) {
  double cur_time = current_timestamp();
  if (utility_mode == AUTO_MODE) {
    if (pkt_lost) {
      is_uniform.update(rtt_acked);
    }
    if (!rtt_window.is_copa(rtt_acked, cur_time)) {
      if (operation_mode == DEFAULT_MODE)
        cout << "Switched to loss sensitive mode." << endl;
      operation_mode = LOSS_SENSITIVE_MODE;
    }
    else {
      if (operation_mode == LOSS_SENSITIVE_MODE)
        cout << "Switched to default mode." << endl;
      operation_mode = DEFAULT_MODE;
      delta = 1;
    }
  }
  
  if (utility_mode == TCP_COOP || operation_mode == LOSS_SENSITIVE_MODE) {
    if (prev_delta_update_time == 0)
      delta = 1;
    if (pkt_lost && cur_intersend_time != 0 && prev_delta_update_time_loss + cur_rtt < cur_time) {
      cout << delta << " " << cur_intersend_time << endl;
      double median_rtt = 1 / (delta * cur_intersend_time);
      if (true || cur_rtt > median_rtt) // congestive loss
        delta *= 2;
      else
        delta /= 2;
      prev_delta_update_time_loss = cur_time;
    }
    else {
      if (prev_delta_update_time + cur_rtt < cur_time) {
	delta = 1 / (1 + 1 / delta);
	prev_delta_update_time = cur_time;
      }
    }
    cout << "DU " << cur_time << " " << flow_id << " " << delta << endl;
    //delta = min(delta, 1.0);
    //delta = max(delta, 0.1);
  }
}

double MarkovianCC::randomize_intersend(double intersend) {
  //return rand_gen.exponential(intersend);
  //return rand_gen.uniform(0.99*intersend, 1.01*intersend);
  return intersend;
}


void MarkovianCC::update_intersend_time() {
  double cur_time __attribute((unused)) = current_timestamp();
  if (external_min_rtt == 0) {
    cout << "External min. RTT estimate required." << endl;
    exit(1);
  }
  
  // First two RTTs are for probing
  if (num_pkts_acked < 2 * num_probe_pkts - 1)
    return;

  // Calculate useful quantities
  double rtt_ewma = rtt_acked;
  double queuing_delay = rtt_ewma - min_rtt;
  double target_window;
  if (queuing_delay == 0)
    target_window = numeric_limits<double>::max();
  else
    target_window = rtt_ewma / (queuing_delay * delta);

  // Handle start-up behavior
  if (slow_start) {
    if (do_slow_start || target_window == numeric_limits<double>::max()) {
      _the_window += 1;
      if (_the_window >= target_window)
        slow_start = false;
    }
    else {
      //_the_window = target_window * delta;
      _the_window = rtt_ewma / ((min_rtt + rtt_window.get_max()) * 0.5 - min_rtt);
      cout << "Fast Start: " << rtt_window.get_min() << " " << rtt_window.get_max() << " " << _the_window << endl;
      slow_start = false;
    }
  }
  // Update the window
  else {
    if (last_update_time + rtt_ewma < cur_time) {
      if (abs(update_dir) > 2 * pkts_per_rtt / 3) {
        if (prev_update_dir * update_dir > 0)
          update_amt *= 2;
        else {
          update_amt = 1;
          prev_update_dir *= -1;
        }
      }
      else
        update_amt = 1;
      last_update_time = cur_time;
      pkts_per_rtt = update_dir = 0;
    }
    update_amt = min(update_amt, (int)_the_window);
    ++ pkts_per_rtt;

    if (last_update_time + rtt_ewma < cur_time) {
      if (_the_window < target_window) {
        ++ update_dir;
        _the_window += update_amt / _the_window;
      }
      else {
        -- update_dir;
        _the_window -= update_amt / _the_window;
      }
    }    
    else {
      if (_the_window < target_window) {
        _the_window += 1 / (delta * _the_window);
      }
      else {
        _the_window -= 1 / (delta * _the_window);
      }
    }
  }

  // Set intersend time and perform boundary checks.
  _the_window = max(2.0, _the_window);
  cur_intersend_time = rtt_ewma / _the_window;
  _intersend_time = randomize_intersend(cur_intersend_time);
  //cout << cur_time << " " << _the_window << " " << rtt_ewma << " " << min_rtt << endl;
}

void MarkovianCC::onACK(int ack, 
			double receiver_timestamp __attribute((unused)),
			double sent_time, int delta_class __attribute((unused))) {
  int seq_num = ack - 1;
  double cur_time = current_timestamp();

  assert(cur_time > sent_time);

  if (rtt_acked == 0 || num_pkts_acked < num_probe_pkts - 1)
    rtt_acked.force_set(cur_time - sent_time, cur_time / min_rtt);
  rtt_acked.update(cur_time - sent_time, cur_time / min_rtt);
  min_rtt = min(min_rtt, cur_time - sent_time);
  rtt_window.update_window_size(10000.0);
  rtt_window.update(cur_time - sent_time, cur_time);
  assert(rtt_acked >= min_rtt);

  if (rtt_long_avg == 0 || num_pkts_acked < num_probe_pkts - 1)
    rtt_long_avg.force_set(cur_time - sent_time, cur_time / min_rtt);
  rtt_long_avg.update(cur_time - sent_time, cur_time / min_rtt);  
  // loss_rate = loss_rate * (1.0 - alpha_loss);
  rtt_var.update(abs(rtt_long_avg - cur_time + sent_time), cur_time / min_rtt);

  if (prev_ack_time != 0) {
    interarrival_ewma.update(cur_time - prev_ack_time, cur_time / min_rtt);
    interarrival.push(cur_time - prev_ack_time);
  }
  prev_ack_time = cur_time;

  update_delta(false, cur_time - sent_time);
  update_intersend_time();

  bool pkt_lost = false;
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
      prev_rtt = x.second.rtt;
      prev_rtt_update_time = x.second.sent_time;
      prev_avg_sending_rate = x.second.prev_avg_sending_rate;
      if (x.first < seq_num) {
	++ num_pkts_lost;
	pkt_lost = true;
      }
      unacknowledged_packets.erase(x.first);
    }
  }
  if (pkt_lost) {
    //cout << "Seq Num Loss Detect " << rtt_acked << endl;
    //update_delta(true);
  }

  ++ num_pkts_acked;
}

void MarkovianCC::onPktSent(int seq_num) {
  double cur_time = current_timestamp();
  // double tmp_prev_avg_sending_rate = 0.0;
  // if (prev_intersend_time != 0.0)
  //   tmp_prev_avg_sending_rate = 1.0 / prev_intersend_time;
  unacknowledged_packets[seq_num] = {cur_time,
                                     cur_intersend_time,
                                     intersend_time_vel,
                                     rtt_acked,
                                     unacknowledged_packets.size() / (cur_time - prev_rtt_update_time)
  };

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
  //update_intersend_time();
  ++ num_pkts_sent;

  _intersend_time = randomize_intersend(cur_intersend_time);
}

void MarkovianCC::close() {
}

void MarkovianCC::onDupACK() {
  ///num_pkts_lost ++;
  // loss_rate = 1.0 * alpha_loss + loss_rate * (1.0 - alpha_loss);
  //cout << "Dupack " << rtt_acked << endl;
  slow_start = false;
  update_delta(true);
}

void MarkovianCC::onTimeout() {
  //num_pkts_lost ++;
  // loss_rate = 1.0 * alpha_loss + loss_rate * (1.0 - alpha_loss);
  //cout << "Timeout\n";
  slow_start = false;
  //update_delta(true);
}

void MarkovianCC::interpret_config_str(string config) {
  // Format - [do_ss:][keep_ext_min_rtt:]delta_update_type:param1:param2...
  // Delta update types:
  //	 -- constant_delta - params:- delta value
  //	 -- pfabric_fct - params:- none
  //	 -- bounded_delay - params:- delay bound (s)
  //	 -- bounded_delay_end - params:- delay bound (s), done in an end-to-end manner

  if (config.substr(0, 6) == "do_ss:") {
    do_slow_start = true;
    config = config.substr(6, string::npos);
    cout << "Will do slow start" << endl;
  }
  if (config.substr(0, 17) == "keep_ext_min_rtt:") {
    keep_ext_min_rtt = true;
    config = config.substr(17, string::npos);
    cout << "Will keep external Min. RTT" << endl;
  }
  
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
    cout << "Bounding delay to " << delay_bound << " s" << endl;
  }
  else if (config.substr(0, 18) == "bounded_delay_end:") {
    utility_mode = BOUNDED_DELAY_END;
    delay_bound = stof(config.substr(18, string::npos).c_str());
    cout << "Bounding delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 19) == "bounded_qdelay_end:") {
    utility_mode = BOUNDED_QDELAY_END;
    delay_bound = stof(config.substr(19, string::npos).c_str());
    cout << "Bounding queuing delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 18) == "bounded_fdelay_end:") {
    utility_mode = BOUNDED_FDELAY_END;
    delay_bound = stof(config.substr(18, string::npos).c_str());
    cout << "Bounding fractional queuing delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 14) == "max_throughput") {
    utility_mode = MAX_THROUGHPUT;
    cout << "Maximizing throughput" << endl;
  }
  else if (config.substr(0, 16) == "different_deltas") {
    utility_mode = CONSTANT_DELTA;
    delta = flow_id * 0.5;
    cout << "Setting constant delta to " << delta << endl;
  }
  else if (config.substr(0, 8) == "tcp_coop") {
    utility_mode = TCP_COOP;
    cout << "Cooperating with TCP" << delta << endl;
  }
  else if (config.substr(0, 14) == "const_behavior") {
    utility_mode = CONST_BEHAVIOR;
    behavior = stof(config.substr(15, string::npos).c_str());
    cout << "Exhibiting constant behavior " << behavior << endl;
  }
  else if (config.substr(0, 14) == "auto") {
    utility_mode = AUTO_MODE;
    cout << "Using Automatic Mode" << endl;
  }
  else {
    utility_mode = CONSTANT_DELTA;
    delta = 1.0;
    cout << "Incorrect format of configuration string '" << config
	 << "'. Using constant delta mode with delta = 1 by default" << endl;
  }
}
