#ifndef UTILITIES
#define UTILITIES

#include <cassert>
#include <cmath>

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

	operator double() const {return ewma;}
	// true if update has been called atleast once
	bool is_valid() const {return denominator != 0.0;}
};
#endif