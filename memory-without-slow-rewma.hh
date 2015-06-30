#ifndef MEMORY_WITHOUT_SLOW_REWMA_HH
#define MEMORY_WITHOUT_SLOW_REWMA_HH

#include <vector>
#include <string>
#include <boost/functional/hash.hpp>
#include <cassert>

#include "packet.hh"
#include "dna.pb.h"

// Keeps track of the state variable for a particular sender.
class Memory {
public:
  typedef double DataType;

private:
  DataType _rec_send_ewma;
  DataType _rec_rec_ewma;
  DataType _rtt_ratio;
  DataType _slow_rec_rec_ewma;

  double _last_tick_sent;
  double _last_tick_received;
  double _last_receiver_timestamp;
  double _min_rtt;

  unsigned long _num_packets_received;
  unsigned long _num_packets_lost;

public:
  Memory( const std::vector< DataType > & s_data )
    : _rec_send_ewma( s_data.at( 0 ) ),
      _rec_rec_ewma( s_data.at( 1 ) ),
      _rtt_ratio( s_data.at( 2 ) ),
      //_slow_rec_rec_ewma( s_data.at( 3 ) ),
      _slow_rec_rec_ewma( 0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _last_receiver_timestamp( 0 ),
      _min_rtt( 0 ),
      _num_packets_received( 0 ),
      _num_packets_lost( 0 )
  {}

  Memory()
    : _rec_send_ewma( 0 ),
      _rec_rec_ewma( 0 ),
      _rtt_ratio( 0.0 ),
      _slow_rec_rec_ewma( 0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _last_receiver_timestamp( 0 ),
      _min_rtt( 0 ),
      _num_packets_received( 0 ),
      _num_packets_lost( 0 )
  {}

  void reset( void ) { _rec_send_ewma = _rec_rec_ewma = _rtt_ratio = _slow_rec_rec_ewma = _last_tick_sent = _last_tick_received = _min_rtt = 0; }

  static const unsigned int datasize = 3;

  const DataType & field( unsigned int num ) const { assert(num <= 2); return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : _rtt_ratio ; }
  DataType & mutable_field( unsigned int num )     { assert(num <= 2); return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : _rtt_ratio ; }

  // Should be called when a packet is about to be sent. This does not do anything now
  void packet_sent( const Packet & packet __attribute((unused)) ) {}
  // Should be called with all the packets that are received. This Updates the memory values
  void packets_received( const std::vector< Packet > & packets, const unsigned int flow_id, const double link_rate_normalizing_factor );
  void advance_to( const unsigned int tickno __attribute((unused)) ) {}

  std::string str( void ) const;

  // compares all tracked values. Does not compare loss rate yet, as it is not yet used
  bool operator>=( const Memory & other ) const { return (_rec_send_ewma >= other._rec_send_ewma) && (_rec_rec_ewma >= other._rec_rec_ewma) && (_rtt_ratio >= other._rtt_ratio); }
  // compares all tracked values
  bool operator<( const Memory & other ) const { return (_rec_send_ewma < other._rec_send_ewma) && (_rec_rec_ewma < other._rec_rec_ewma) && (_rtt_ratio < other._rtt_ratio); }
  // compares all tracked values
  bool operator==( const Memory & other ) const { return (_rec_send_ewma == other._rec_send_ewma) && (_rec_rec_ewma == _rec_rec_ewma) && (_rtt_ratio == other._rtt_ratio); }

  RemyBuffers::Memory DNA( void ) const;
  Memory( const bool is_lower_limit, const RemyBuffers::Memory & dna );

  friend size_t hash_value( const Memory & mem );
};

extern const Memory & MAX_MEMORY( void );

#endif