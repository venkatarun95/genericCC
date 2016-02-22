#ifndef RANDOM_HH
#define RANDOM_HH

#include <boost/random/mersenne_twister.hpp>

#include <random>

typedef boost::random::mt19937 PRNG;

extern PRNG & global_PRNG();

class RandGen {
	std::default_random_engine generator;
	
 public:
	RandGen();

	double uniform(double a, double b);
	double exponential(double lambda);
};

#endif
