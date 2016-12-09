#include <iostream>
#include <limits>

#include "utilities.hh"

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
  ValType largest[window_len+1];
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
  //assert(timestamp >= loss_events.back());
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
