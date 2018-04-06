#include <iostream>
#include <limits>

#include "estimators.hh"

using namespace std;

void TimeEwma::reset() {
	ewma = denominator = last_update_timestamp = 0.0;
}

void TimeEwma::update(double value, double timestamp) {
	assert(timestamp >= last_update_timestamp);
	// assert (value >= 0); // we don't support negative values because we use that to detect errors

	if (denominator == 0.0) {
		ewma = value;
		denominator = 1.0;
		last_update_timestamp = timestamp;
		return;
	}
	if (denominator > 0.01*std::numeric_limits<double>::max()) {
		cout << "Resetting denominator" << endl;
		denominator = 1; // reset ewma calculation
	}

	double ewma_factor = pow(alpha, timestamp - last_update_timestamp);
	double new_denom = 1.0 + ewma_factor * denominator;
	double new_ewma = (value + ewma_factor * ewma * denominator) / new_denom;
  if (!((value < ewma && new_ewma < ewma) || (value >= ewma && new_ewma >= ewma)))
    return; // Happens due to floating point error sometimes for small 'value'. Ignore
	assert((value < ewma && new_ewma < ewma) || (value >= ewma && new_ewma >= ewma));
	ewma = new_ewma;
	denominator = new_denom;
	last_update_timestamp = timestamp;	
}

void TimeEwma::force_set(double value, double timestamp) {
	ewma = value;
	denominator = 1;
	last_update_timestamp = timestamp;
}


/*************************** PERCENTILE *******************************/
void Percentile::push(ValType val) {
	window.push(val);
	if (window.size() > window_len)
		window.pop();
}

Percentile::ValType Percentile::get_percentile_value() {
	int N = (1.0 - percentile) * window_len;
	ValType largest[window_len+1]; // Last element is only for making code shorter
	for (int i = 0; i < N; i++) largest[i] = -1;

	if (window.size() == 0)
		return 0;

	for (int i = 0;i < window_len; i++) {
		ValType val = window.front(), x = val;
		window.pop();
		for (int j = 0;j < N; j++) {
			if (x <= largest[N-1]) continue;
			if (x > largest[j]) {
				ValType t = largest[j];
				largest[j] = x;
				x = t;
			}
		}
		window.push(val);
	}
	return largest[N-1];
}

void Percentile::reset() {
  while (!window.empty())
    window.pop();
}


/*********************** LOSS RATE ESTIMATE ***************************/
void LossRateEstimate::update(bool lost) {
  if (lost) {
    if (cur_loss_interval <= 2) return;
    loss_events.push_back(cur_loss_interval);
    cout << cur_loss_interval << endl;
    cur_loss_interval = 0;
    if ((int)loss_events.size() > window)
      loss_events.pop_front();
  }
  else
    ++ cur_loss_interval;
}

double LossRateEstimate::value() {
  double sum = 0.0, sum_weight = 0.0;
  int i = 0;
  if (loss_events.size() == 0) return 0.0;
  int cur_window = (int)loss_events.size();
  for (auto x = loss_events.rbegin(); x != loss_events.rend(); ++x) {
    double weight = 1.0;
    if (i >= cur_window / 2)
      weight = 1.0 - (i + 1.0 - cur_window / 2.0) / (cur_window / 2.0 + 1);
    sum += weight * (*x);
    sum_weight += weight;
  }
  if (sum / sum_weight < cur_loss_interval) {
    sum += cur_loss_interval;
    sum_weight += 1;
  }
  return sum_weight / sum;
}

bool ReduceOnLoss::update(bool loss, double cur_time, double rtt) {
  if (loss)
    ++ num_lost;
  ++ num_pkts;

  if (cur_time > prev_win_time + 2 * rtt && num_pkts > 20) {
    double loss_rate = 1.0 * num_lost / num_pkts;
    //cout << "Losses: " << num_lost << " " << num_pkts << endl;
    prev_win_time = cur_time;
    num_lost = 0;
    num_pkts = 0;
    if (loss_rate > 0.3)
      return true;
  }
  return false;
}

void ReduceOnLoss::reset() {
  num_lost = 0;
  num_pkts = 0;
  prev_win_time = 0.;
}

/*************************** TIME WINDOW ******************************/

TimeWindow::TimeWindow(double window_size) :
  window_size(window_size),
  window()
{}

void TimeWindow::update(double data, double time) {
  window.push_back(make_pair(time, data));
  while (!window.empty() && window.front().first < time - window_size)
    window.pop_front();
}

void TimeWindow::update_window_size(double new_window_size) {
  window_size = new_window_size;
}

double TimeWindow::get_min() const {
  if (window.empty())
    return 0;
  double res = numeric_limits<double>::max();
  for (const auto& x : window)
    if (x.second < res)
      res = x.second;
  return res;
}

double TimeWindow::get_max() const {
  if (window.empty())
    return 0;
  double res = numeric_limits<double>::min();
  for (const auto& x : window)
    if (x.second > res)
      res = x.second;
  return res;
}

bool TimeWindow::is_copa(double rtt, double cur_time) const {
  double min_rtt = numeric_limits<double>::max();
  double max_rtt = numeric_limits<double>::min();
  for (auto x = window.rbegin(); x != window.rend(); ++x) {
    if (x->first > cur_time - 4*rtt)
      max_rtt = max(max_rtt, x->second);
    min_rtt = min(min_rtt, x->second);
  }

  bool before = false, after = false;
  double threshold = min_rtt + 0.1 * (max_rtt - min_rtt);
  int num_before = 0, num_after = 0;
  for (auto x = window.rbegin(); x != window.rend(); ++x) {
    if (x->first > cur_time - 2*rtt) {
      if (x->second < threshold)
        after = true;
      ++ num_after;
    }
    else if (x->first > cur_time - 4*rtt) {
      if (x->second < threshold)
        before = true;
      ++ num_before;
    }
    else
      break;
  }
  if (num_after < 4 || num_before < 4)
    return true;
  return before && after;
}

bool TimeWindow::empty() const {
  return window.empty();
}

void TimeWindow::reset() {
  while (!window.empty()) {
    window.pop_front();
  }
}

/******************** Is Uniform Distribution? ************************/

// Returns n choose k
double combinatoral_nck(int n, int k) {
  double res = 1;
  if (k > n/2) k = n-k;
  for (int i = n-k+1; i <= n; ++i)
    res *= i;
  for (int i = 1; i <= k; ++i)
    res /= i;
  return res;
}

IsUniformDistr::IsUniformDistr(int window_len) :
  window_len(window_len),
  window(),
  sum(0),
  precomputed()
{
  for (int i = 0; i <= window_len; ++i) {
    precomputed.push_back(*(new vector<double>()));
    for (int j = 0; j <= i; ++j) {
      double confidence = 0;
      for (int k = 0; k < j; ++k)
        confidence += combinatoral_nck(i, k);
      for (int k = 0; k < i; ++k)
        confidence /= 2.0;
      precomputed[i].push_back(confidence);
    }
  }
}

void IsUniformDistr::update(double data) {
  window.push_back(data);
  sum += data;
  if (window.size() > (size_t)window_len) {
    sum -= window.front();
    window.pop_front();
  }
}

double IsUniformDistr::get_confidence(const TimeWindow& rtt_window) const {
  if (window.empty() || rtt_window.empty())
    return 0.0;
  int num_greater = 0;
  double avg __attribute((unused)) = sum / window.size();
  double mid = 0.5 * (rtt_window.get_min() + rtt_window.get_max());
  int num_seen = 0;
  double max_confidence = 0.0;
  for (auto it = window.rbegin(); it != window.rend(); ++it) {
    if (*it > mid)
      ++ num_greater;
    ++ num_seen;
    double confidence = precomputed[num_seen][num_greater];
    if (confidence > max_confidence)
      max_confidence = confidence;
  }
  return max_confidence;
}

void IsUniformDistr::reset() {
  while (!window.empty())
    window.pop_back();
  sum = 0;
}
