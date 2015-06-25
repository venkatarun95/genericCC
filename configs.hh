// Type of remy (in terms of the congestion signals) to be used
//
// Options are:
//   - REMYTYPE_DEFAULT
//   - REMYTYPE_LOSS_SIGNAL
//   - REMYTYPE_WITHOUT_SLOW_REWMA
#define REMYTYPE_LOSS_SIGNAL

// Controls the scaling of the rate-based congestion control signals
//
// Scaling is based on link rate of the bottleneck link measured using the 'packet-pair' trick. Rate is measured in packets/s
#undef SCALE_SEND_RECEIVE_EWMA
#define NUM_PACKETS_PER_LINK_RATE_MEASUREMENT 5
// the lower the value, the slower the exponential averaging
#define LINK_RATE_MEASUREMENT_ALPHA (1.0/16.0)
// in number of packets per second
#define TRAINING_LINK_RATE (4000000.0/1500.0)