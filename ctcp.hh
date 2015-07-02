#ifndef REMY_TCP_HH
#define REMY_TCP_HH

#include <assert.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "ccc.hh"
#include "remycc.hh"
#include "tcp-header.hh"
#include "udp-socket.hh"

using namespace std;

#define packet_size 1500
#define data_size 1400

template <class T>
class CTCP{
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
    tot_packets_transmitted( 0 )
  {
    socket.bindsocket( ipaddr, port, myaddr, myport );
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
    tot_packets_transmitted( 0 )
  {
    socket.bindsocket( dstaddr, dstport, myaddr, myport );
  }

  //duration in milliseconds
  void send_data ( double duration, int flow_id, int src_id );

  void listen_for_data ( );
};

#include <string.h>
#include <stdio.h>

#include "configs.hh"

using namespace std;

// use the packet 'pair' trick to measure link rate of the bottleneck link. This parameter controls the number of packets used for that purpose. The constant is defined in configs.hh
const int num_packets_per_link_rate_measurement = NUM_PACKETS_PER_LINK_RATE_MEASUREMENT;
const double link_rate_measurement_alpha = LINK_RATE_MEASUREMENT_ALPHA;

double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
  using namespace chrono;
  high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
  return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

template<class T>
void CTCP<T>::send_data( double duration, int flow_id, int src_id ){
  TCPHeader header, ack_header;

  // this is the data that is transmitted. A sizeof(TCPHeader) header followed by a sring of dashes
  char buf[packet_size];
  memset(buf, '-', sizeof(char)*packet_size);
  buf[packet_size-1] = '\0';

  // for link logging
  ofstream link_logfile;
  if( LINK_LOGGING )
    link_logfile.open( LINK_LOGGING_FILENAME, ios::out | ios::app );

  // for flow control
  _last_send_time = 0.0;
  double cur_time = 0;
  // note: this is not the sequence number that is actually transmitted. packets are transmitted in groups of num_packets_per_link_rate_measurement (say n) numbered as seq_num*n, seq_num*n + 1, ..., seq_num*n + (n-1)
  int seq_num = 0;
  _last_send_time = 0.0;
  _largest_ack = -1;
  _packets_sent = 0;

  // variables for link rate measurement
  int cur_ack_group_number = -1;
  int num_packets_received_in_current_group = -1;
  double latest_ack_time_in_group = -1;
  double link_rate_measurement_accumulator = -1; // for accumulating before finding average
  double last_measured_link_rate = -1; // in packets/s. In case num_packets_per_link_rate_measurement == 1, this value will be maintained as -1.

  // for maintaining performance statistics
  double delay_sum = 0;
  int num_packets_transmitted = 0;
  int transmitted_bytes = 0;

  cout << "Assuming training link rate of: " << TRAINING_LINK_RATE << " pkts/sec" << endl;
  congctrl.init();
  chrono::high_resolution_clock::time_point start_time_point = chrono::high_resolution_clock::now();

  while ( cur_time < duration ){
    cur_time = current_timestamp( start_time_point );
    assert( _packets_sent >= _largest_ack );
    // Warning: The number of unacknowledged packets may exceed the congestion window by num_packets_per_link_rate_measurement
    while ( ( (_packets_sent < _largest_ack + 1 + congctrl.get_the_window()) 
      and (_last_send_time + congctrl.get_intersend_time()*num_packets_per_link_rate_measurement <= cur_time) ) 
      and cur_time < duration ){
      
      for (  int i = 0;i < num_packets_per_link_rate_measurement; i++ ) {
        header.seq_num = seq_num * num_packets_per_link_rate_measurement + i;
        header.flow_id = flow_id;
        header.src_id = src_id;
        header.sender_timestamp = cur_time;
        header.receiver_timestamp = 0;

        memcpy( buf, &header, sizeof(TCPHeader) );
        socket.senddata( buf, packet_size, NULL );

        _packets_sent++;

        cur_time = current_timestamp( start_time_point );
      }
      congctrl.onPktSent( header.seq_num );

      cur_time = current_timestamp( start_time_point );
      _last_send_time = cur_time;
      seq_num++; // a set of num_packets_per_link_rate_measurement (say 3) are sequence numbered as 3n, 3n+1, 3n+2. This is later used for link rate measurement while receiving packets.

      //cout<<congctrl.get_the_window()<<" "<<congctrl.get_intersend_time()<<endl;
    }

    cur_time = current_timestamp( start_time_point );
    double timeout = _last_send_time + congctrl.get_timeout(); // everything in milliseconds
    if( congctrl.get_the_window() > 0 )
      timeout = min( timeout, _last_send_time + congctrl.get_intersend_time()*num_packets_per_link_rate_measurement - cur_time );

    sockaddr_in other_addr;
    if( socket.receivedata( buf, packet_size, timeout, other_addr ) == 0 ) {
      cur_time = current_timestamp( start_time_point );
      if( cur_time > _last_send_time + congctrl.get_timeout() )
        congctrl.onTimeout();
      continue;
    }

    memcpy(&ack_header, buf, sizeof(TCPHeader));
    ack_header.seq_num ++; // because the receiver doesn't do that for us yet

    if ( ack_header.src_id != src_id || ack_header.flow_id != flow_id ){
      if( ack_header.src_id != src_id ){
        std::cerr<<"Received incorrect ack for src "<<ack_header.src_id<<" to "<<src_id<<endl;
      }
      continue;
    }

    cur_time = current_timestamp( start_time_point );

    // measure link speed
    assert( num_packets_per_link_rate_measurement > 0 );
    if( num_packets_per_link_rate_measurement > 1){
      int ack_group_num = ( ack_header.seq_num - 1) / num_packets_per_link_rate_measurement;
      int ack_group_seq = ( ack_header.seq_num - 1) % num_packets_per_link_rate_measurement;
      // Note: in case of reordering, the measurement is cancelled. Measurement will be bad in case network reorders extensively
      if ( ack_group_num == cur_ack_group_number ) { 
        if ( ack_group_seq == num_packets_received_in_current_group ){
          num_packets_received_in_current_group += 1;
          link_rate_measurement_accumulator += cur_time - latest_ack_time_in_group;
          latest_ack_time_in_group = cur_time;
          if( num_packets_received_in_current_group == num_packets_per_link_rate_measurement ){
            if ( last_measured_link_rate < 0 ) // set initial value
              last_measured_link_rate = ( 1000 / ( link_rate_measurement_accumulator / (num_packets_per_link_rate_measurement - 1) ) );
            else
              last_measured_link_rate = last_measured_link_rate*(1 - link_rate_measurement_alpha) + link_rate_measurement_alpha*( 1000 / ( link_rate_measurement_accumulator / (num_packets_per_link_rate_measurement - 1) ) );
            congctrl.onLinkRateMeasurement( last_measured_link_rate );

            // Log measured link speed and last RTT
            if( LINK_LOGGING )
              link_logfile << cur_time << " " << last_measured_link_rate << " " << cur_time - ack_header.sender_timestamp << endl;
            
            // Inform the congestion control protocol. Note: each group of 
            // NUM_PACKETS_PER_LINK_RATE_MEASUREMENT is treated as one packet 
            // as far as the congestion control protocol is concerned. This 
            // must be compensated for in the CC, for example in RemyCC
            congctrl.onACK( ack_header.seq_num, ack_header.receiver_timestamp );
          }
        }
      }
      else if ( ack_group_num > cur_ack_group_number ){
        cur_ack_group_number = ack_group_num;
        num_packets_received_in_current_group = 1;
        link_rate_measurement_accumulator = 0;
        latest_ack_time_in_group = cur_time;
      }
    }

    // Track performance statistics
    delay_sum += cur_time - ack_header.sender_timestamp;
    this->tot_delay += cur_time - ack_header.sender_timestamp;

    transmitted_bytes += data_size;
    this->tot_bytes_transmitted += data_size;

    num_packets_transmitted += 1;
    this->tot_packets_transmitted += 1;
    
    // Inform our congestion control protocol
    _largest_ack = max(_largest_ack, ack_header.seq_num);
  }

  cur_time = current_timestamp( start_time_point );

  congctrl.close();

  this->tot_time_transmitted += cur_time;

  double throughput = transmitted_bytes/( cur_time / 1000.0 );
  double delay = (delay_sum / 1000) / num_packets_transmitted;

  std::cout<<"\n\nData Successfully Transmitted\n\tThroughput: "<<throughput<<" bytes/sec\n\tAverage Delay: "<<delay<<" sec/packet\n";

  double avg_throughput = tot_bytes_transmitted / ( tot_time_transmitted / 1000.0);
  double avg_delay = (tot_delay / 1000) / tot_packets_transmitted;
  std::cout<<"\n\tAvg. Throughput: "<<avg_throughput<<" bytes/sec\n\tAverage Delay: "<<avg_delay<<" sec/packet\n";

  if( LINK_LOGGING )
    link_logfile.close();
}

template<class T>
void CTCP<T>::listen_for_data ( ){

}

#endif