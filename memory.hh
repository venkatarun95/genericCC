#ifndef MEMORY_HH
#define MEMORY_HH

#include "configs.hh"

#ifdef REMYTYPE_DEFAULT

#include "memory-default.hh"

#elif REMYTYPE_LOSS_SIGNAL

#warning Link rate has not been normalized in these files!
#include "memory-with-loss-signal.hh"

#elif REMYTYPE_WITHOUT_SLOW_REWMA

#warning Link rate has not been normalized in these files!
#include "memory-without-slow-rewma.hh"

#else

#error Please define a remy type

#endif

#endif