#include <algorithm>
#include <limits>

#include "rat.hh"

using namespace std;

Rat::Rat( WhiskerTree & s_whiskers, const bool s_track )
  :  _whiskers( s_whiskers ),
     _memory(),
     _packets_sent( 0 ),
     _packets_received( 0 ),
     _track( s_track ),
     _last_send_time( 0 ),
     _the_window( 0 ),
     _intersend_time( 0 ),
     _flow_id( 0 ),
     _largest_ack( -1 )
{
}

void Rat::packets_received( const vector< Packet > & packets, const double link_rate_normalizing_factor ) {
  _packets_received += packets.size();
  
  int flow_id = -1;
  assert( packets.size() );
  for ( auto &packet : packets ){
    _largest_ack = max( packet.seq_num, _largest_ack );
    flow_id = packet.flow_id;
  }
  assert( flow_id != -1 );

  _memory.packets_received( packets, flow_id/*_flow_id*/, link_rate_normalizing_factor );

  const Whisker & current_whisker( _whiskers.use_whisker( _memory, _track ) );

  _the_window = current_whisker.window( _the_window );
  _intersend_time = current_whisker.intersend();
}

void Rat::reset( const double & )
{
  _memory.reset();
  _last_send_time = 0;
  _the_window = 0;
  _intersend_time = 0;
  _flow_id++;
  _largest_ack = _packets_sent - 1; /* Assume everything's been delivered */
  assert( _flow_id != 0 );
}

double Rat::next_event_time( const double & tickno ) const
{
  if ( _packets_sent < _largest_ack + 1 + _the_window ) {
    if ( _last_send_time + _intersend_time <= tickno ) {
      return tickno;
    } else {
      return _last_send_time + _intersend_time;
    }
  } else {
    /* window is currently closed */
    return std::numeric_limits<double>::max();
  }
}

// returns whether or not a packet should be sent. If yes, updates internal state accordingly
bool Rat::send( const double & curtime )
{
  assert( _packets_sent >= _largest_ack + 1 );

  if ( _the_window == 0 ) {
    /* initial window and intersend time */
    const Whisker & current_whisker( _whiskers.use_whisker( _memory, _track ) );
    _the_window = current_whisker.window( _the_window );
    _intersend_time = current_whisker.intersend();
    // assert(_the_window != 0 ); //edit - venkat - just to ensure that a sender doesn't stay 0 forever because right now, I believe that memory will never be called if no packets are sent. But something tells me that my understanding is incorrect
  }

  if ( (_packets_sent < _largest_ack + 1 + _the_window)
       and (_last_send_time + _intersend_time <= curtime) ) {

    //Packet p( id, _flow_id, curtime, _packets_sent );
    _packets_sent++;
    //_memory.packet_sent( p );
    //next.accept( p, curtime );
    _last_send_time = curtime;
    return true;
  }

  return false;
}