#include "rtt-window.hh"

#include <cassert>
#include <limits>

using namespace std;

// Find extreme value in window
class ExtremeWindow {
  // Whether to find minimum or maximum
  bool find_min;
  // The maximum time till which to maintain window
  double max_time;
  // Format: (timestamp, val)
  deque<pair<double, double>> vals;

  void clear_old_hist(double now);
 public:
  ExtremeWindow(double max_time);
  void update_max_time(double t);
  void new_sample(double val, double timestamp);
  // Return the maintained extreme value
  operator double() const;

};

RTTWindow::RTTWindow()
  : max_time(10e3),
    min_rtt(numeric_limits<double>::max()),
    //unjittered_rtt(numeric_limits<double>::max()),
    srtt(0.),
    srtt_alpha(1. / 16.),
    rtts()
{}

void RTTWindow::clear_old_hist(double now) {
  // Whether or not min. RTT needs to be recomputed
  bool recompute_min_rtt = false;

  // Delete all samples older than max_time. However, if there is only one
  // sample left, don't delete it
  while(rtts.size() > 1 && rtts.front().first < now - max_time) {
    if (rtts.front().second <= min_rtt)
      recompute_min_rtt = true;
    rtts.pop_front();
  }

  // Recompute min RTT if necessary
  if (recompute_min_rtt) {
    // Since `rtts` is a monotonically increasing array, the value at the front
    // contains the minimum
    min_rtt = rtts.front().second;
  }
}

void RTTWindow::new_rtt_sample(double rtt, double now) {
  // Delete any RTT samples immediately before this one that are greater than
  // the current value
  while (!rtts.empty() && rtts.back().second > rtt)
    rtts.pop_back();

  // Push back current sample and update min. RTT
  rtts.push_back(make_pair(now, rtt));
  if (rtt < min_rtt)
    min_rtt = rtt;

  // Update smoothed RTT
  srtt = srtt_alpha * rtt + (1. - srtt_alpha) * srtt;

  // Delete unnecessary history
  clear_old_hist(now);

  max_time = min(10e3, 20 * srtt);
}

double RTTWindow::get_min_rtt() const {
  return min_rtt;
}

double RTTWindow::get_unjittered_rtt(double now) const {
  // TODO(venkat): make more efficient by maintaining a pointer to the
  // appropriate element
  double res = numeric_limits<double>::max();
  for (auto it = rtts.rbegin(); it != rtts.rend(); ++it) {
    res = min(res, (*it).second);
    if ((*it).first < now - srtt * 0.5)
      break;
  }
  assert(res != numeric_limits<double>::max());
  return res;
}

bool RTTWindow::is_copa(double now) const {
  const double num_rtts = 6.;
  // TODO(venkat): make more efficient by maintaining a pointer to the
  // appropriate element
  double min = numeric_limits<double>::max();
  double max = -1.;
  for (auto it = rtts.rbegin(); it != rtts.rend(); ++it) {
    res = min(res, (*it).second);
    if ((*it).first < now - srtt * num_rtts)
      break;
  }
  assert(min != numeric_limits<double>::max());
  assert(max != -1.);
  double threshold = get_min_rtt() + 0.1 * (max - get_min_rtt());
  return min < threshold;
}
