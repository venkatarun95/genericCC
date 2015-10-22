#ifndef CTCP_HH
#define CTCP_HH

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "tcp-header.hh"
#include "udp-socket.hh"

using namespace std;

#define packet_size 1500
#define data_size (packet_size-sizeof(TCPHeader))

template <class T>
class CTCP {
public:
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

  int _packets_sent;
  int _largest_ack;

  double tot_time_transmitted;
  double tot_delay;
  int tot_bytes_transmitted;
  int tot_packets_transmitted;

  // buffer used for storing send and received packets
  char buf[packet_size];
  // time at which the latest flow started
  chrono::high_resolution_clock::time_point start_time_point;

  int flow_id;
  int src_id;

  void tcp_handshake();
  // Only function that actually sends packets
  void send_packet(int seq_num);

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
    _packets_sent( 0 ),
    _largest_ack( -1 ),
    tot_time_transmitted( 0 ),
    tot_delay( 0 ),
    tot_bytes_transmitted( 0 ),
    tot_packets_transmitted( 0 ),
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
    _packets_sent( 0 ),
    _largest_ack( -1 ),
    tot_time_transmitted( 0 ),
    tot_delay( 0 ),
    tot_bytes_transmitted( 0 ),
    tot_packets_transmitted( 0 ),
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
