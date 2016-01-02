#ifndef UTILITIES_HH
#define UTILITIES_HH

#include <cassert>
#include <math.h>

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

  void force_set(const double value, const double tmp __attribute((unused))) {
    valid = true;
    ewma = value;
  }

  void update(const double value, const double tmp __attribute((unused))) {
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
#endif
