#ifndef MEMORY_CC
#define MEMORY_CC

#ifdef REMYTYPE_DEFAULT

#include "memory-default.cc"

#elif REMYTYPE_LOSS_SIGNAL

#include "memory-with-loss-signal.cc"

#elif REMYTYPE_WITHOUT_SLOW_REWMA

#include "memory-without-slow-rewma.cc"

#else

#error Please define a remy type

#endif

#endif