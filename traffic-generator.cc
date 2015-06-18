/*#include "traffic-generator.hh"

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
				_ctcp.send_data( on_duration, flow_id, id );

				++ flow_id;
				std::cout<<"Sender: "<<id<<", Flow: "<<flow_id<<". Transmitted "<<on_duration<<" bytes."<<endl<<std::flush;
			}
		break;
	}
}

template<class T>
void TrafficGenerator<T>::spawn_senders( int num_senders ){
	PRNG prng( global_PRNG() );
//	for( int i = 0;i < num_senders; i++ ){
//		CTCP connection ( _ctcp );
//		int seed = boost::random::uniform_int_distribution<>()( prng );
//		_senders.push_back ( std::thread( &TrafficGenerator<T>::send_data, this, seed, i ) );
//		//std::thread( &TrafficGenerator<T>::send_data, this, seed, i );
//	}

//	for ( auto & thread : _senders ){
//		thread.join();
//	}
	
	// Have only one sender for now, since the NAT creates issues for multiple senders anyway
	assert( num_senders == 1); // feature not yet implemented
	
	int seed = boost::random::uniform_int_distribution<>()( prng );
	send_data( seed, 0 );
}*/