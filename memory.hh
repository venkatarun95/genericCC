#ifndef MEMORY_HH
#define MEMORY_HH

#include "configs.hh"

#ifdef REMYTYPE_DEFAULT
#define REMYTYPE_DEFINED
#include "memory-default.hh"
#endif



#ifdef REMYTYPE_LOSS_SIGNAL

#ifdef REMYTYPE_DEFINED
#error Only one remy type should be defined 
#endif

#define REMYTYPE_DEFINED
#include "memory-with-loss-signal.hh"

#endif /* REMYTYPE_LOSS_SIGNAL */



#ifdef REMYTYPE_WITHOUT_SLOW_REWMA

#ifdef REMYTYPE_DEFINED
#error Only one remy type should be defined 
#endif

#define REMYTYPE_DEFINED
#include "memory-without-slow-rewma.hh"

#endif /* REMYTYPE_WITHOUT_SLOW_REWMA */

#ifndef REMYTYPE_DEFINED
#error Please define a remy type
#endif

#endif /* MEMORY_HH */