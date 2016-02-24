#ifndef CONFIGS_HH
#define CONFIGS_HH

#include <string>

// Type of remy (in terms of the congestion signals) to be used
//
// Options are:
//   - REMYTYPE_DEFAULT
//   - REMYTYPE_LOSS_SIGNAL
//   - REMYTYPE_WITHOUT_SLOW_REWMA
#define REMYTYPE_DEFAULT

// Controls the scaling of the rate-based congestion control signals
//
// Scaling is based on link rate of the bottleneck link measured using 
// the 'packet-pair' trick. Rate is measured in packets/s
#undef SCALE_SEND_RECEIVE_EWMA
#define NUM_PACKETS_PER_LINK_RATE_MEASUREMENT 1

// The lower the value, the slower the exponential averaging
#define LINK_RATE_MEASUREMENT_ALPHA (1.0/64.0)
// Link rate for which the remy was trained on (in packets per second)
extern double TRAINING_LINK_RATE; // declared in sender.cc

// Miscellaneous Controls
// If true, logs bottleneck link rate and rtt for each 'packet pair' 
// sent along with a timestamp. Declared in sender.cc
extern bool LINK_LOGGING;
extern std::string LINK_LOGGING_FILENAME;


// Some Types
// Used for sequence/ack number, data length etc.
typedef int32_t NumBytes;
// Magic number used to identify module type to be used, for example
// to select congestion control protocol, sendw window manager etc.
typedef const int16_t MagicId;

// Some Constants
constexpr NumBytes snd_window_size = 0xffff;

#endif
