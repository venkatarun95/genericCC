#include "utilities.hh"

void TimeEwma::reset() {
	ewma = denominator = last_update_timestamp = 0.0;
}

void TimeEwma::update(double value, double timestamp) {
	assert(timestamp >= last_update_timestamp);

	if (denominator == 0.0) {
		ewma = value;
		denominator = 1.0;
		last_update_timestamp = timestamp;
		return;
	}

	double ewma_factor = pow(alpha, timestamp - last_update_timestamp);
	double new_denom = 1.0 + ewma_factor * denominator;
	ewma = (value + ewma_factor * ewma * denominator) / new_denom;
	denominator = new_denom;
	last_update_timestamp = timestamp;
}