#ifndef MEMORY_WITH_LOSS_SIGNAL_HH
#define MEMORY_WITH_LOSS_SIGNAL_HH

#include <boost/functional/hash.hpp>
#include <vector>
#include <queue>
#include <cassert>
#include <iostream>
#include <string>

#include "packet.hh"
#include "dna.pb.h"

#include "configs.hh"

// Keeps track of the state variable for a particular sender.
class Memory {
public:
  typedef double DataType;

private:
  DataType _rec_send_ewma;
  DataType _rec_rec_ewma;
  DataType _rtt_ratio;
  DataType _slow_rec_rec_ewma; //although this is present and calculated, it does not leave this class
  DataType _loss_rate;

  double _last_tick_sent;
  double _last_tick_received;
  double _last_receiver_timestamp;
  double _min_rtt;

  // for having packet loss as a signal
  double _rtt_estimate; 
  std::queue< Packet > _lost_packets;
  std::queue< Packet > _all_packets_in_rtt_window;
  int _largest_ack;

public:
  Memory( const std::vector< DataType > & s_data )
    : _rec_send_ewma( s_data.at( 0 ) ),
      _rec_rec_ewma( s_data.at( 1 ) ),
      _rtt_ratio( s_data.at( 2 ) ),
      //_slow_rec_rec_ewma( s_data.at( 3 ) ),
      _slow_rec_rec_ewma( 0 ), //just a stopgap initial value
      _loss_rate( s_data.at( 3 ) ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _last_receiver_timestamp( 0 ),
      _min_rtt( 0 ),
      _rtt_estimate( 0 ),
      _lost_packets( ),
      _all_packets_in_rtt_window( ),
      _largest_ack( 0 )
  {}

  Memory()
    : _rec_send_ewma( 0 ),
      _rec_rec_ewma( 0 ),
      _rtt_ratio( 0.0 ),
      _slow_rec_rec_ewma( 0 ),
      _loss_rate( 0 ),
      _last_tick_sent( 0 ),
      _last_tick_received( 0 ),
      _last_receiver_timestamp( 0 ),
      _min_rtt( 0 ),
      _rtt_estimate( 0 ),
      _lost_packets( ),
      _all_packets_in_rtt_window( ),
      _largest_ack( 0 )
  {}

  void reset( void ) { 
    _rec_send_ewma = _rec_rec_ewma = _rtt_ratio = _slow_rec_rec_ewma = _loss_rate = _last_tick_sent = _last_tick_received = _min_rtt = _rtt_estimate = _largest_ack = 0; 
    while ( !_lost_packets.empty() )
      _lost_packets.pop();
    while( !_all_packets_in_rtt_window.empty() )
      _all_packets_in_rtt_window.pop();
  }

  static const unsigned int datasize = 4;

  const DataType & field( unsigned int num ) const { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio : _loss_rate ; }
  DataType & mutable_field( unsigned int num )     { return num == 0 ? _rec_send_ewma : num == 1 ? _rec_rec_ewma : num == 2 ? _rtt_ratio : _loss_rate ; }

  // Should be called when a packet is about to be sent. This does not do anything now
  //void packet_sent( const Packet & packet __attribute((unused)) ) {}
  // Should be called with all the packets that are received. This Updates the memory values
  void packets_received( const std::vector< Packet > & packets, const unsigned int flow_id, const double link_rate_normalizing_factor );
  void advance_to( const unsigned int tickno __attribute((unused)) ) {}

  std::string str( void ) const;

  // compares all tracked values. Does not compare loss rate yet, as it is not yet used
  bool operator>=( const Memory & other ) const { return (_rec_send_ewma >= other._rec_send_ewma) && (_rec_rec_ewma >= other._rec_rec_ewma) && (_rtt_ratio >= other._rtt_ratio) && (_loss_rate >= other._loss_rate); }
  // compares all tracked values
  bool operator<( const Memory & other ) const { return (_rec_send_ewma < other._rec_send_ewma) && (_rec_rec_ewma < other._rec_rec_ewma) && (_rtt_ratio < other._rtt_ratio) && (_loss_rate < other._loss_rate); }
  // compares all tracked values
  bool operator==( const Memory & other ) const { return (_rec_send_ewma == other._rec_send_ewma) && (_rec_rec_ewma == _rec_rec_ewma) && (_rtt_ratio == other._rtt_ratio) && (_loss_rate == other._loss_rate); }

  RemyBuffers::Memory DNA( void ) const;
  Memory( const bool is_lower_limit, const RemyBuffers::Memory & dna );

  friend size_t hash_value( const Memory & mem );
};

extern const Memory & MAX_MEMORY( void );

#endif