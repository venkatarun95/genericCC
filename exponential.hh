#ifndef EXPONENTIAL_HH
#define EXPONENTIAL_HH

#include <boost/random/exponential_distribution.hpp>

#include "random.hh"


//	A simple wrapper for the boost exponential distribution generator
//	 usage:	Exponential exp_distribution_;
//          double val = exp_distribution_.sample();
class Exponential
{
private:
  boost::random::exponential_distribution<> distribution;

  PRNG & prng;

public:
  Exponential( const double & rate, PRNG & s_prng ) : distribution( rate ), prng( s_prng ) {}
  
  double sample( void ) { return distribution( prng ); }
};

#endif
