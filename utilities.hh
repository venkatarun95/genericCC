#ifndef UTILITIES_HH
#define UTILITIES_HH

#include <cassert>
#include <math.h>
#include <queue>

class TimeEwma {
	double ewma;
	double denominator;
	double alpha;
	double last_update_timestamp;

 public:
	// lower the alpha, slower the moving average
	TimeEwma(const double s_alpha)
	:	ewma(0.0),
		denominator(0.0),
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
		if (valid)
			ewma = alpha * value + (1.0 - alpha) * ewma;
		else
			ewma = value;
    valid = true;
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
// computed. The window contains values younger than a given constant.
class WindowAverage {
	// Format: (value, timestamp)
	std::queue< std::pair<double, double> > window;
	double window_size; // In time units
	double sum;

 public:
	WindowAverage(double window_size)
		: window(),
			window_size(window_size),
			sum()
	{}

	void force_set(double value, double timestamp) {
		reset();
		update(value, timestamp);
	}

	void update(double value, double timestamp) {
		window.push(std::make_pair(value, timestamp));
		sum += value;
		while (window.front().second < timestamp - window_size && window.size() > 1) {
			sum -= window.front().first;
			window.pop();
		}
	}

	void round() {assert(false);}

	void reset() {
		while (!window.empty())
			window.pop();
		sum = 0.0;
	}

	operator double() const {
		if (window.size() == 0) return 0.0;
		return sum / window.size();
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
	static constexpr double percentile = 0.95;
	typedef double ValType;

	std::queue<ValType> window;

 public:
	Percentile() : window() {}

	void push(ValType val);
	ValType get_percentile_value();
};

#endif
