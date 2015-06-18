#ifndef TRAFFIC_GENERATOR_HH
#define TRAFFIC_GENERATOR_HH

#include <thread>
#include <vector>

#include <boost/random/uniform_int_distribution.hpp>

#include "ctcp.hh"
#include "exponential.hh"
#include "random.hh"
#include "remycc.hh"

enum TrafficType {EXPONENTIAL_ON_OFF};

template<class T>
class TrafficGenerator{
private:
	TrafficType _traffic_type;
	union TrafficParams {
		struct{
			double _mean_off_duration;
			double _mean_on_duration;
		} _exponential_on_off;
	} _traffic_params;

	T &_ctcp;

	std::vector< std::thread > _senders;

	void send_data( int seed, int id );

public:
	TrafficGenerator( T &s_ctcp, double s_mean_on_duration, double s_mean_off_duration, TrafficType s_traffic_type )
		:	_traffic_type( s_traffic_type ),
			_traffic_params( { s_mean_off_duration, s_mean_on_duration } ),
			_ctcp( s_ctcp ),
			_senders()
	{
		_traffic_params._exponential_on_off._mean_on_duration = s_mean_on_duration;
		_traffic_params._exponential_on_off._mean_off_duration = s_mean_off_duration;
	}

	void spawn_senders( int num_senders );
};

template<class T>
void TrafficGenerator<T>::send_data( int seed, int id ){
	PRNG prng( seed );
	switch( _traffic_type ){
		case TrafficType::EXPONENTIAL_ON_OFF:
			Exponential on ( 1 / _traffic_params._exponential_on_off._mean_on_duration, prng );
			Exponential off ( 1 / _traffic_params._exponential_on_off._mean_off_duration, prng );

			int flow_id = 0;
			while(1){
				double off_duration = off.sample( );
				//double off_duration = _traffic_params._exponential_on_off._mean_off_duration;
				std::this_thread::sleep_for( std::chrono::duration<double, std::milli>( unsigned(off_duration) ) );

				//WARNING: Right now I am switching on for a number of KILO BYTES. It is not time switched
				double on_duration = on.sample( );
				//double on_duration = 1000.0 * _traffic_params._exponential_on_off._mean_on_duration;
				std::cout<<"Switching on";
				_ctcp.send_data( on_duration, flow_id, id );
				std::cout<<"Switching off";

				++ flow_id;
				std::cout<<"Sender: "<<id<<", Flow: "<<flow_id<<". Transmitted for "<<on_duration<<" ms."<<endl<<std::flush;
			}
		break;
	}
}

template<class T>
void TrafficGenerator<T>::spawn_senders( int num_senders ){
	PRNG prng( global_PRNG() );
/*	for( int i = 0;i < num_senders; i++ ){
		CTCP connection ( _ctcp );
		int seed = boost::random::uniform_int_distribution<>()( prng );
		_senders.push_back ( std::thread( &TrafficGenerator<T>::send_data, this, seed, i ) );
		//std::thread( &TrafficGenerator<T>::send_data, this, seed, i );
	}

	for ( auto & thread : _senders ){
		thread.join();
	}*/
	
	// Have only one sender for now, since the NAT creates issues for multiple senders anyway
	assert( num_senders == 1); // feature not yet implemented
	
	int seed = boost::random::uniform_int_distribution<>()( prng );
	int src_id = boost::random::uniform_int_distribution<>()( prng );
	std::cout<<"Assigning Source ID: "<<src_id<<std::endl;
	send_data( seed, src_id );
}

#endif