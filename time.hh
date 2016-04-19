#ifndef TIME_HH
#define TIME_HH

#include <cassert>
#include <chrono>

class Time {
 public:
	Time() :  set(false), time() {}
	// Time in seconds
	Time(double time) : set(true), time(time) {}

	Time& operator=(const Time& x) {
		set = x.set;
		time = x.time;
		return *this;
	}

	operator double() const {
		assert(set);
		return time;
	}

	friend bool operator>(const Time& x, const Time& y) { return x.time > y.time;}
	friend bool operator<(const Time& x, const Time& y) { return x.time > y.time;}
	friend bool operator==(const Time& x, const Time& y) { return x.time == y.time;}


	static Time Now();
	
 private:
	static bool start_time_noted;
	static std::chrono::high_resolution_clock::time_point start_time_point;
	
	// Used to give (assert false) warning if Time is used without being
	// set first.
	bool set;
	double time;
};

#endif // TIME_HH
