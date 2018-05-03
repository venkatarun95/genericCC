#ifndef ESTIMATORS_HH
#define ESTIMATORS_HH

#include <cassert>
#include <deque>
#include <iostream>
#include <list>
#include <math.h>
#include <queue>
#include <tuple>
#include <vector>

class TimeEwma {
	double ewma;
	double denominator;
	double alpha;
	double last_update_timestamp;

 public:
	// lower the alpha, slower the moving average
	TimeEwma(const double s_alpha)
	:	ewma(),
		denominator(),
		alpha(1.0 - s_alpha),
		last_update_timestamp()
	{
		assert(alpha < 1.0);
	}

	void reset();
	void update(double value, double timestamp);
	void add(double value) {ewma += value;}
	void round() {ewma = int(ewma*100000) / 100000.0;}
	void force_set(double value, double timestamp);

	operator double() const {return ewma;}
	// true if update has been called atleast once
	bool is_valid() const {return denominator != 0.0;}
};

class PlainEwma {
  double ewma;
  double alpha;
  bool valid;

 public:
  PlainEwma(const double alpha)
  : ewma(),
    alpha(alpha),
    valid(false)
  {}

  // Note: The tmp argument in the functions below is to maintain compatibility
  // with TimeEwma

  void force_set(const double value, const double tmp __attribute((unused)) = 0) {
    valid = true;
    ewma = value;
  }

  void update(const double value, const double tmp __attribute((unused)) = 0) {
    valid = true;
    ewma = alpha * value + (1.0 - alpha) * ewma;
  }

  void round() { 
    ewma = int(ewma * 100000) / 100000.0;
  }
  void reset() { 
    ewma = 0.0; 
  }
  operator double() const { return ewma; }
  bool is_valid() const { return valid; }
};

// Maintains a sliding window of values for which the average is
// computed. The window contains values younger than a given
// constant. Computes a time average, so each time instant is given
// equal weightage.
class WindowAverage {
	// Format: (value, timestamp)
	std::queue< std::pair<double, double> > window;
	double window_size; // In time units
	double sum;
	// Timestamp of the value last popped. If 0.0, no value has been
	// popped yet.
	double prev_popped_timestamp;

 public:
	WindowAverage(double window_size)
		: window(),
			window_size(window_size),
			sum(0.0),
			prev_popped_timestamp(0.0)
	{}

	void force_set(double value, double timestamp) {
		reset();
		update(value, timestamp);
	}

	void update(double value, double timestamp) {
		if (window.size() == 0) {
			// Push two nearby values into the window
			window.push(std::make_pair(value, timestamp - 1e-3));
			assert(prev_popped_timestamp == 0.0);
			update(value, timestamp);
			return;
		}
		sum += value * (timestamp - window.back().second);
		window.push(std::make_pair(value, timestamp));

		// std::cout << "Sum = " << sum << " for " << window.size() << " at " << double(*(this)) << std::endl;

		while (window.front().second < timestamp - window_size && window.size() > 2) {
			// std::cout << "Deleting: " << window.front().first << " " << sum << " " << window.front().second << " " << timestamp << " " << window_size << " at " << double(*(this)) << std::endl;
			if (prev_popped_timestamp != 0.0)
				sum -= window.front().first * (window.front().second - prev_popped_timestamp);
			prev_popped_timestamp = window.front().second;
			window.pop();
		}
	}

	void round() {assert(false);}

	void reset() {
		while (!window.empty())
			window.pop();
		sum = 0.0;
		prev_popped_timestamp = 0.0;
	}

	operator double() const {
		assert(window.size() != 1);
		if (window.size() < 2)return 0.0;
		return sum / (window.back().second - window.front().second);
	}

	bool valid() const {
		return !window.empty();
	}
};

// Maintains the x percentile value over a small window
class Percentile {
  // The following can be dynamic too. Constant makes it a little more
  // efficient and to emphasize that the implementation is not
  // efficient for large window sizes.
  static constexpr int window_len = 100;
  double percentile;
  typedef double ValType;
  
  std::queue<ValType> window;

 public:
  Percentile(double percentile) :
    percentile(percentile),
    window()
  {}
  
  void push(ValType val);
  ValType get_percentile_value();
  void reset();
};

// Estimates the packet loss rate for TCP friendliness. Uses the
// method described in "Equation Based Congestion Control for Unicast
// Applications"
class LossRateEstimate {
  const int window = 8;
  std::list<double> loss_events;
  int cur_loss_interval;

public:
  LossRateEstimate() : loss_events(), cur_loss_interval() {}

  void reset() {loss_events.clear(); cur_loss_interval = 0; }
	void update(bool lost);
	double value();
	// true if update has been called atleast once
	bool is_valid() const {return loss_events.size() == (unsigned)window;}

};

// Returns true once per RTT if loss rate for that RTT is greater than 10%
class ReduceOnLoss {
  int num_lost;
  int num_pkts;
  double prev_win_time;

public:
  ReduceOnLoss()
    : num_lost(0),
      num_pkts(0),
      prev_win_time(0.0)
  {}

  // Returns whether to reduce or not
  bool update(bool loss, double cur_time, double rtt);
  void reset();
};

// Maintains data-points in a given time window and provides
// statistics on that.
class TimeWindow {
  double window_size;
  // Pair of (time, data) values.
  std::deque< std::pair<double, double> > window;

public:
  TimeWindow(double window_size);

  // Add a new data-point to the window. Assumes time is an increasing
  // function.
  void update(double data, double time);
  void update_window_size(double new_window_size);
  double get_min() const;
  double get_max() const;
  bool is_copa(double rtt, double cur_time) const;
  bool empty() const;
  void reset();
};

// Uses an unspecified statistical analysis method to determine
// whether a moving window of values is uniformly distributed between
// its minimum and maximum values.
//
// Reports true or false based on whether the confidence is greater
// than a given alpha.
class IsUniformDistr {
  int window_len;
  std::deque<double> window;

  // Sum of values in the window. Used to compute average.
  double sum;
  // Precomputed confidence values given window size and number of
  // 'trues' in binomial hypothesis testing.
  std::vector< std::vector<double> > precomputed;

 public:
  IsUniformDistr(int window_len);
  
  // Adds new datapoint.
  void update(double data);
  // Returns confidence of whether or not the data is uniformly distributed.
  double get_confidence(const TimeWindow& rtt_window) const;
  void reset();
};

#endif // ESTIMATORS_HH
