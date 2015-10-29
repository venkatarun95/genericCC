#ifndef CTCP_HH
#define CTCP_HH

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "tcp.hh"
#include "udp-socket.hh"

using namespace std;

template <class T>
class CTCP {
public:
  static const unsigned packet_size = 1500;
  static const unsigned data_size = (packet_size-16);
  enum ConnectionType{ SENDER, RECEIVER };

private:
  T congctrl;
  UDPSocket socket;
  ConnectionType conntype;

  string myaddr;
  int myport;

  string dstaddr;
  int dstport;

  double _last_send_time;

  // buffer used for storing send and received packets
  char buf[CTCP::packet_size];
  // time at which the latest flow started
  chrono::high_resolution_clock::time_point start_time_point;

  int flow_id;
  int src_id;

  void tcp_handshake();
  // Only function that actually sends packets
  void send_packet(const TcpHeader& header);

public:

  CTCP( T s_congctrl, string ipaddr, int port, string s_myaddr, int s_myport )
  :   congctrl( s_congctrl ),
    socket(),
    conntype( SENDER ),
    myaddr( s_myaddr ),
    myport( s_myport ),
    dstaddr( ipaddr ),
    dstport( port ),
    _last_send_time( 0.0 ),
    // buf(nullptr),
    start_time_point(),
    flow_id( 0 ),
    src_id( 0 )
  {
    socket.bindsocket( ipaddr, port, myaddr, myport );
    memset(buf, '-', sizeof(char)*packet_size);
    buf[packet_size-1] = '\0';
  }

  CTCP( CTCP<T> &other )
  : congctrl( other.congctrl ),
    socket(),
    conntype( other.conntype ),
    myaddr( other.myaddr ),
    myport( other.myport ),
    dstaddr( other.dstaddr ),
    dstport( other.dstport ),
    _last_send_time( 0.0 ),
    start_time_point(),
    flow_id( 0 ),
    src_id( 0 )
  {
    socket.bindsocket( dstaddr, dstport, myaddr, myport );
  }

  // Main function to call to send data
  //
  // flow_size -- in bytes or ms
  // byte_switched -- if true interpret flow_size as bytes, interpret
  //     as ms otherwise
  // flow_id, src_id -- are put inside the packet headers
  void send_data ( double flow_size, bool byte_switched, int s_flow_id, int s_src_id );

  void listen_for_data ( );
};

#endif
