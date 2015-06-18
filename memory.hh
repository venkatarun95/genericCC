#ifndef MEMORY_HH
#define MEMORY_HH

#ifdef REMYTYPE_DEFAULT

#include "memory-default.hh"

#elif REMYTYPE_LOSS_SIGNAL

#include "memory-with-loss-signal.hh"

#elif REMYTYPE_WITHOUT_SLOW_REWMA

#include "memory-without-slow-rewma.hh"

#else

#error Please define a remy type

#endif

#endif