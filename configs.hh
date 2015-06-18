// Type of remy (in terms of the congestion signals) to be used
//
// Options are:
//   - REMYTYPE_DEFAULT
//   - REMYTYPE_LOSS_SIGNAL
//   - REMYTYPE_WITHOUT_SLOW_REWMA
#define REMYTYPE_DEFAULT

// Controls the scaling of the rate-based congestion control signals
//
// Scaling is based on link rate of the bottleneck link measured using the 'packet-pair' trick. Rate is measured in packets/s
#define SCALE_SEND_RECEIVE_EWMA
#define NUM_PACKETS_PER_LINK_RATE_MEASUREMENT 5
#define LINK_RATE_MEASUREMENT_ALPHA 1.0/16.0
#define TRAINING_LINK_RATE (4000000.0/1500.0)