#ifndef RAT_HH
#define RAT_HH

#include <vector>
#include <string>
#include <limits>

#include "packet.hh"
#include "whiskertree.hh"
#include "memory.hh"

class Rat
{
private:
  const WhiskerTree & _whiskers;
  Memory _memory;

  int _packets_sent, _packets_received;

  bool _track;

  double _last_send_time;

  int _the_window;
  double _intersend_time;

  unsigned int _flow_id;
  // This represents the largest sequence number from among the packets recieved (via packets_recieved) from SenderGang. So this is not the ACK in the traditional sense but is 
  int _largest_ack;

public:
  Rat( WhiskerTree & s_whiskers, const bool s_track=false );

  void packets_received( const std::vector< Packet > & packets, const double link_rate_normalizing_factor );
  void reset( const double & tickno ); /* start new flow */

  bool send( const double & curtime );

  const WhiskerTree & whiskers( void ) const { return _whiskers; }

  Rat & operator=( const Rat & ) { assert( false ); return *this; }

  double next_event_time( const double & tickno ) const;

  const int & packets_sent( void ) const { return _packets_sent; }

  int cur_window_size() const { return _the_window; }
  double cur_intersend_time() const {return _intersend_time; }
};

#endif