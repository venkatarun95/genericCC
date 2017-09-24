#ifndef TRAFFIC_GENERATOR_HH
#define TRAFFIC_GENERATOR_HH

#include <thread>
#include <vector>

#include <boost/random/uniform_int_distribution.hpp>

#include "exponential.hh"
#include "random.hh"
#include "remycc.hh"


template<class T>
class TrafficGenerator{
private:
	enum TrafficType {EXPONENTIAL_ON_OFF, DETERMINISTIC_ON_OFF};
	enum SwitchType {TIME_SWITCHED, BYTE_SWITCHED};

	TrafficType _traffic_type;
	SwitchType _switch_type;

	union TrafficParams {
		struct{
			double _mean_off_unit;
			double _mean_on_unit;
			unsigned int num_cycles;
		} _on_off;
	} _traffic_params;

	T &_ctcp;

	std::vector< std::thread > _senders;

	void send_data(int seed, int id);

public:
	TrafficGenerator(T &s_ctcp, double s_mean_on_unit, double s_mean_off_unit, std::string traffic_params)
		:	_traffic_type(TrafficType::EXPONENTIAL_ON_OFF),
			_switch_type(SwitchType::TIME_SWITCHED),
			_traffic_params({ s_mean_off_unit, s_mean_on_unit, (unsigned int)-1 }),
			_ctcp(s_ctcp),
			_senders()
	{
		_traffic_params._on_off._mean_on_unit = s_mean_on_unit;
		_traffic_params._on_off._mean_off_unit = s_mean_off_unit;

		// parse traffic_params
		size_t start_pos = 0;
		while (start_pos < traffic_params.length()) {
			size_t end_pos = traffic_params.find(',', start_pos);
			if (end_pos == std::string::npos)
				end_pos = traffic_params.length();
			
			std::string arg = traffic_params.substr(start_pos, end_pos - start_pos);
			if (arg == "exponential")
				_traffic_type = TrafficType::EXPONENTIAL_ON_OFF;
			else if (arg == "deterministic")
				_traffic_type = TrafficType::DETERMINISTIC_ON_OFF;
			else if (arg == "byte_switched"){
				_switch_type = SwitchType::BYTE_SWITCHED;
			}
			else if (arg.substr(0, 11) == "num_cycles=")
				_traffic_params._on_off.num_cycles = (unsigned int) \
					atoi(arg.substr(11).c_str());
			else 
				std::cout << "Unrecognised parameter: " << arg << endl;

			start_pos = end_pos + 1;
		}
	}

	void spawn_senders(int num_senders);
};

template<class T>
void TrafficGenerator<T>::send_data(int seed, int id) {
	PRNG prng(seed);
	Exponential on (1 / _traffic_params._on_off._mean_on_unit, prng);
	Exponential off (1 / _traffic_params._on_off._mean_off_unit, prng);

	unsigned int flow_id = 0;
	while (1) {
		double off_duration = off.sample();
		double on_duration = on.sample();

		if (_traffic_type == TrafficType::DETERMINISTIC_ON_OFF) {
			off_duration = _traffic_params._on_off._mean_off_unit;
			on_duration = _traffic_params._on_off._mean_on_unit;
		}

		std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(unsigned(off_duration)));

		bool byte_switched = (_switch_type == SwitchType::BYTE_SWITCHED);
		_ctcp.send_data(on_duration, byte_switched, flow_id, id);

		++ flow_id;
		std::cout<<"Sender: "<<id<<", Flow: "<<flow_id<<". Transmitted for "<<on_duration<<(byte_switched?" bytes.":" ms.")<<endl<<std::flush;

		if (flow_id >= _traffic_params._on_off.num_cycles)
			break;
	}
}

template<class T>
void TrafficGenerator<T>::spawn_senders(int num_senders) {
	PRNG prng(global_PRNG());
	
	// Have only one sender for now, since the NAT creates issues for multiple senders anyway
	assert(num_senders == 1); // feature not yet implemented
	
	int seed = boost::random::uniform_int_distribution<>()(prng);
	int src_id = boost::random::uniform_int_distribution<>()(prng);
	std::cout<<"Assigning Source ID: "<<src_id<<std::endl;
	send_data(seed, src_id);
}

#endif
