
#include <cstdio>
#include <cstring>
#include <thread>

#include "ctcp.hh"
#include "configs.hh"

using namespace std;

// use the packet train trick to measure link rate of the bottleneck
// link. This parameter controls the number of packets used for that
// purpose. The constants are defined in configs.hh
const int num_packets_per_link_rate_measurement = NUM_PACKETS_PER_LINK_RATE_MEASUREMENT;
const double link_rate_measurement_alpha = LINK_RATE_MEASUREMENT_ALPHA;

double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
  using namespace chrono;
  high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
  return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000; //convert to milliseconds, because that is the scale on which the rats have been trained
}

template<class T>
void CTCP<T>::tcp_handshake() {
  TCPHeader header, ack_header;

  header.seq_num = -1;
  header.flow_id = -1;
  header.src_id = -1;
  header.sender_timestamp = -1;
  header.receiver_timestamp = -1;

  memcpy( buf, &header, sizeof(TCPHeader) );
  socket.senddata( buf, packet_size, NULL );

  sockaddr_in other_addr;
  while ( true ) {
    if (socket.receivedata( buf, packet_size, 2000, other_addr ) == 0) {
      cerr << "Could not establish connection" << endl;
      continue;
    }
    memcpy(&ack_header, buf, sizeof(TCPHeader));
    if (ack_header.seq_num != -1 || ack_header.flow_id != -1)
      continue;
    if (ack_header.sender_timestamp != -1 || ack_header.src_id != -1)
      continue;
    break;
  }
  cout << "Connection Established." << endl;
}

template<class T>
void CTCP<T>::send_packet(int seq_num) {
  TCPHeader header;
  header.seq_num = seq_num;
  header.flow_id = flow_id;
  header.src_id = src_id;
  header.sender_timestamp = current_timestamp( start_time_point );
  header.receiver_timestamp = 0;

  memcpy( buf, &header, sizeof(TCPHeader) );
  socket.senddata( buf, packet_size, NULL );
}

// takes flow_size in milliseconds (byte_switched=false) or in bytes (byte_switched=true)
template<class T>
void CTCP<T>::send_data( double flow_size, bool byte_switched, int s_flow_id, int s_src_id ){
  flow_id = s_flow_id;
  src_id = s_src_id;
  tcp_handshake();

  TCPHeader ack_header;

  // for flow control
  _last_send_time = 0.0;
  double cur_time = 0;
  int seq_num = 0;
  _last_send_time = 0.0;
  _largest_ack = -1;
  _packets_sent = 0;

  // for maintaining performance statistics
  double delay_sum = 0;
  int num_packets_transmitted = 0;
  int transmitted_bytes = 0;

  // for detecting when packets are not sent on time
  // double total_intersend_backlog = 0;
  // const double alpha_avg_intersend_error_perc = 1.0 / 2048.0;

  cout << "Assuming training link rate of: " << TRAINING_LINK_RATE \
    << " pkts/sec" << endl;
  congctrl.init();
  start_time_point = \
    chrono::high_resolution_clock::now();

  while ((byte_switched?(num_packets_transmitted*data_size):cur_time) < flow_size) {
    cur_time = current_timestamp( start_time_point );
    assert( _packets_sent >= _largest_ack );

    // send packets if required
    // cout << cur_time << " " << (_last_send_time + congctrl.get_intersend_time() <= cur_time) << endl;
    while ((_packets_sent < _largest_ack + 1 + congctrl.get_the_window())
      and (_last_send_time + congctrl.get_intersend_time() <= cur_time)) {

      // see if packet is being sent at the right time
      // if (100*(cur_time - _last_send_time - congctrl.get_intersend_time()) / congctrl.get_intersend_time() > 1)
      //   cout << _last_send_time + congctrl.get_intersend_time() << " " << cur_time << " " << 100*(cur_time - _last_send_time - congctrl.get_intersend_time()) / congctrl.get_intersend_time() << endl;

      send_packet(seq_num);

      congctrl.onPktSent( seq_num );
      _packets_sent++;

      cur_time = current_timestamp( start_time_point );

      _last_send_time += congctrl.get_intersend_time();
      seq_num++;

      // Uncomment for logging
      // cout<<congctrl.get_the_window()<<" "<<congctrl.get_intersend_time()<<" "<<last_measured_link_rate<<endl;
    }

    // decide timeout
    cur_time = current_timestamp(start_time_point);
    double timeout = congctrl.get_intersend_time(); // everything in milliseconds
    if( congctrl.get_the_window() > 0 )
      timeout = min(timeout, congctrl.get_intersend_time());

    // timeout -= avg_intersend_error_perc*timeout;
    // timeout = max(0.0, timeout);
    timeout = 0.0;
    // cout << timeout << " " << congctrl.get_intersend_time() << endl;

    // wait for packets
    sockaddr_in other_addr;
    if ( socket.receivedata(buf, packet_size, timeout, other_addr) == 0 ) {
      // comes here if there is a timeout
      cur_time = current_timestamp( start_time_point );
      if ( cur_time > _last_send_time + congctrl.get_timeout() )
        congctrl.onTimeout();
      this_thread::yield();
      continue;
    }

    // process ack
    memcpy(&ack_header, buf, sizeof(TCPHeader));
    ack_header.seq_num ++; // because the receiver doesn't do that for us yet

    if ( ack_header.src_id != src_id || ack_header.flow_id != flow_id ){
      if( ack_header.src_id != src_id ){
        std::cerr<<"Received incorrect ack for src "<<ack_header.src_id<<" to "<<src_id<<endl;
      }
      continue;
    }
    cur_time = current_timestamp( start_time_point );

    // Track performance statistics
    delay_sum += cur_time - ack_header.sender_timestamp;
    this->tot_delay += cur_time - ack_header.sender_timestamp;

    transmitted_bytes += data_size;
    this->tot_bytes_transmitted += data_size;

    num_packets_transmitted += 1;
    this->tot_packets_transmitted += 1;


    // measure link speed
    #ifdef SCALE_SEND_RECEIVE_EWMA
        assert(false);
    #endif
    congctrl.onACK(ack_header.seq_num, ack_header.receiver_timestamp);

    _largest_ack = max(_largest_ack, ack_header.seq_num);
  }

  cur_time = current_timestamp(start_time_point);

  congctrl.close();

  this->tot_time_transmitted += cur_time;

  double throughput = transmitted_bytes/( cur_time / 1000.0 );
  double delay = (delay_sum / 1000) / num_packets_transmitted;

  std::cout<<"\n\nData Successfully Transmitted\n\tThroughput: "<<throughput<<" bytes/sec\n\tAverage Delay: "<<delay<<" sec/packet\n";

  double avg_throughput = tot_bytes_transmitted / ( tot_time_transmitted / 1000.0);
  double avg_delay = (tot_delay / 1000) / tot_packets_transmitted;
  std::cout<<"\n\tAvg. Throughput: "<<avg_throughput<<" bytes/sec\n\tAverage Delay: "<<avg_delay<<" sec/packet\n";
}

template<class T>
void CTCP<T>::listen_for_data ( ){

}

// Instantiate the templates that will be used
#include "congctrls.hh"
#include "markoviancc.hh"
#include "nashcc.hh"
#include "remycc.hh"

template class CTCP<MarkovianCC>;
template class CTCP<RemyCC>;
template class CTCP<NashCC>;
template class CTCP<DefaultCC>;
