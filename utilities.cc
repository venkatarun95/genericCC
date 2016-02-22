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
	//assert((value < ewma && new_ewma < ewma) || (value >= ewma && new_ewma >= ewma));
	if ((value > ewma && new_ewma < ewma) || (value < ewma && new_ewma > ewma)) {
		cerr << "Ewma overflowed. Resetting" << endl;
		ewma = value;
		denominator = 1;
	}
	else {
		ewma = new_ewma;
		denominator = new_denom;
	}
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
	constexpr int N = (1.0 - percentile) * window_len;
	ValType largest[N+1]; // Last element is only for making code shorter
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
