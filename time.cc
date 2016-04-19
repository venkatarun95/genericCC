#include "time.hh"

using namespace std::chrono;

bool Time::start_time_noted = false;
high_resolution_clock::time_point Time::start_time_point = high_resolution_clock::now();

Time Time::Now() {
	high_resolution_clock::time_point cur_time_point =	\
		high_resolution_clock::now();
	return Time(duration_cast<duration<double>>(cur_time_point - start_time_point).count());
}
