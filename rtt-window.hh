#include <deque>
#include <tuple>

class RTTWindow {
  // Maximum time till which to maintain history. Minimum of 10s and 20 RTTs.
  double max_time;
  // Current estimate of the min RTT
  double min_rtt;
  // Current estimate of the unjittered RTT
  //double unjittered_rtt_ptr;
  // Current estimate of the smoothed RTT
  double srtt;
  const double srtt_alpha;

  // RTT measurements (time of measurement, rtt). For efficiency, only maintains
  // enough measurements to answer specific query (eg. min. RTT in given
  // window). Thus storing only the monotonically increasing subset of RTTs is
  // enough.
  std::deque<std::pair<double, double>> rtts;

  void clear_old_hist(double now);

 public:
  RTTWindow();
  void new_rtt_sample(double rtt, double now);
  double get_min_rtt() const;
  double get_unjittered_rtt(double now) const;
  bool RTTWindow::is_copa(double now) const;
};
