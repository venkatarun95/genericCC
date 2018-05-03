#include <deque>
#include <tuple>

// Private class: Find extreme value in window
class ExtremeWindow {
  // Whether to find minimum or maximum
  bool find_min;
  // The maximum time till which to maintain window
  double max_time;
  // RTT measurements (time of measurement, rtt). For efficiency, only maintains
  // enough measurements to answer specific query (eg. min. RTT in given
  // window). Thus storing only the monotonically increasing (or decreasing of
  // find_min=false)subset of RTTs is enough.
  std::deque<std::pair<double, double>> vals;
  // Computed extreme value
  double extreme;

  void clear_old_hist(double now);
public:
  ExtremeWindow(bool find_min);
  void clear();
  void new_sample(double val, double timestamp);

  void update_max_time(double t) {max_time = t;}
  // Return the maintained extreme value
  operator double() const {return extreme;}

};

class RTTWindow {
  // Current estimate of the smoothed RTT
  double srtt;
  const double srtt_alpha;

  double latest_rtt;
  ExtremeWindow min_rtt;
  ExtremeWindow unjittered_rtt;
  ExtremeWindow is_copa_min;
  ExtremeWindow is_copa_max;

 public:
  RTTWindow();
  void clear();
  void new_rtt_sample(double rtt, double now);

  double get_min_rtt() const;
  double get_unjittered_rtt() const;
  double get_latest_rtt() const;
  bool is_copa() const;
};
