#include "rtt-window.hh"

#include <cassert>
#include <limits>

using namespace std;

ExtremeWindow::ExtremeWindow(bool find_min)
  : find_min(find_min),
    max_time(10e3),
    vals(),
    extreme(find_min?numeric_limits<double>::max():
            numeric_limits<double>::min())
{}

void ExtremeWindow::clear() {
  max_time = 10e3;
  vals.clear();
  extreme = find_min?numeric_limits<double>::max():
    numeric_limits<double>::min();
}

void ExtremeWindow::clear_old_hist(double now) {
  // Whether or not min. RTT needs to be recomputed
  bool recompute = false;

  // Delete all samples older than max_time. However, if there is only one
  // sample left, don't delete it
  while(vals.size() > 1 && vals.front().first < now - max_time) {
    if ((find_min && vals.front().second <= extreme) ||
        (!find_min && vals.front().second >= extreme))
      recompute = true;
    vals.pop_front();
  }

  // Recompute min RTT if necessary
  if (recompute) {
    // Since `rtts` is a monotonically increasing array, the value at the front
    // contains the minimum
    extreme = vals.front().second;
  }
}

void ExtremeWindow::new_sample(double val, double now) {
  // Delete any RTT samples immediately before this one that are greater (or
  // less than if finding max) than the current value
  while (!vals.empty() && ((find_min && vals.back().second > val) ||
                           (!find_min && vals.back().second < val)))
    vals.pop_back();

  // Push back current sample and update extreme
  vals.push_back(make_pair(now, val));
  if ((find_min && val < extreme) || (!find_min && val > extreme))
    extreme = val;

  // Delete unnecessary history
  clear_old_hist(now);
}

RTTWindow::RTTWindow()
  : srtt(0.),
    srtt_alpha(1. / 16.),
    latest_rtt(0.),
    min_rtt(true),
    unjittered_rtt(true),
    is_copa_min(true),
    is_copa_max(false)
{}

void RTTWindow::clear() {
  srtt = 0.;
  min_rtt.clear();
  unjittered_rtt.clear();
  is_copa_min.clear();
  is_copa_max.clear();
}

void RTTWindow::new_rtt_sample(double rtt, double now) {
  // Update smoothed RTT
  if (srtt == 0.)
    srtt = rtt;
  srtt = srtt_alpha * rtt + (1. - srtt_alpha) * srtt;
  latest_rtt = rtt;

  // Update extreme value trackers
  double max_time = max(10e3, 20. * min_rtt);
  min_rtt.update_max_time(max_time);
  unjittered_rtt.update_max_time(min(max_time, srtt * 0.5));
  is_copa_min.update_max_time(min(max_time, srtt * 4));
  is_copa_max.update_max_time(min(max_time, srtt * 4));

  min_rtt.new_sample(rtt, now);
  unjittered_rtt.new_sample(rtt, now);
  is_copa_min.new_sample(rtt, now);
  is_copa_max.new_sample(rtt, now);
}

double RTTWindow::get_min_rtt() const {
  return min_rtt;
}

double RTTWindow::get_unjittered_rtt() const {
  return unjittered_rtt;
}

double RTTWindow::get_latest_rtt() const {
  return latest_rtt;
}

bool RTTWindow::is_copa() const {
  double threshold = min_rtt + 0.1 * (is_copa_max - min_rtt);
  return is_copa_min < threshold;
}
