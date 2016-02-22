#include <chrono>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>

#include "random.hh"

PRNG & global_PRNG( void )
{
  static PRNG generator( time( NULL ) ^ getpid() );
  return generator;
}

RandGen::RandGen()
	:	generator(std::chrono::system_clock::now().time_since_epoch().count())
	{}

double RandGen::uniform(double a, double b) {
	std::uniform_real_distribution<double> x(a, b);
	return x(generator);
}

double RandGen::exponential(double lambda) {
	std::exponential_distribution<double> x(1/lambda);
	return x(generator);
}
