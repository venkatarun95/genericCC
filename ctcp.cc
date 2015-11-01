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
void CTCP<T>::send_packet(const TcpHeader& header) {
  memcpy( buf, &header, sizeof(TcpHeader) );
  socket.senddata( buf, CTCP::packet_size, NULL );
}

// takes flow_size in milliseconds (byte_switched=false) or in bytes (byte_switched=true)
template<class T>
void CTCP<T>::send_data( unsigned flow_size, bool byte_switched, int s_flow_id, int s_src_id ){
  flow_id = s_flow_id;
  src_id = s_src_id;
  TcpConnection conn(src_id, flow_id);
  conn.establish_connection(0.0);

  TcpHeader header;

  // For packet pacing
  _last_send_time = 0.0;
  double cur_time = 0;

  cout << "Assuming training link rate of: " << TRAINING_LINK_RATE \
    << " pkts/sec" << endl;
  cout << "Flow size: " << flow_size << " " << byte_switched << endl;
  congctrl.init();
  start_time_point = \
    chrono::high_resolution_clock::now();

  while (conn.get_state() != TcpConnection::ConnState::CLOSED) {
    cur_time = current_timestamp( start_time_point );

    // send packets if required
    while (((conn.get_snd_window_size() < congctrl.get_the_window())
      && (_last_send_time + congctrl.get_intersend_time() <= cur_time
      )) || conn.transmit_immediately()) {

      unsigned size = CTCP::data_size;
      if (byte_switched)
        size = min(size, flow_size - conn.get_num_bytes_sent());
      header = conn.get_next_pkt(cur_time, size);
      if (!header.valid)
        break;
      if (header.type == TcpHeader::PacketType::SYN && header.size == 0)
        cout << "Trying to establish connection" << endl;
      send_packet(header);

      if (header.size > 0)
        congctrl.onPktSent(header.seq_num);

      cur_time = current_timestamp(start_time_point);

      // We do not take actual time. This way we can compensate for
      // errors in scheduling.
      _last_send_time += congctrl.get_intersend_time();

      // Uncomment for logging
      // cout<<congctrl.get_the_window()<<" "<<congctrl.get_intersend_time()<<" "<<last_measured_link_rate<<endl;
    }

    if ((byte_switched?conn.get_num_bytes_sent():cur_time) >= flow_size) {
      conn.close_connection();
    }

    // decide timeout
    cur_time = current_timestamp(start_time_point);
    double timeout = congctrl.get_intersend_time(); // everything in milliseconds
    if (conn.get_rtx_timeout() != -1.0)
      timeout = min(timeout, conn.get_rtx_timeout() - cur_time);
    if (conn.transmit_immediately())
      timeout = 0;

    if (timeout > 0)
      this_thread::yield();

    // wait for packets
    sockaddr_in other_addr;
    if ( socket.receivedata(buf, CTCP::packet_size, timeout, other_addr) == 0 ) {
      // comes here if there is a timeout
      cur_time = current_timestamp( start_time_point );
      if ( cur_time >= conn.get_rtx_timeout() && conn.get_rtx_timeout() != -1.0)
        congctrl.onTimeout();
      this_thread::yield();
      continue;
    }

    // Process ack
    cur_time = current_timestamp(start_time_point);
    memcpy(&header, buf, sizeof(TcpHeader));
    conn.register_received_packet(header, cur_time);

    // measure link speed
    #ifdef SCALE_SEND_RECEIVE_EWMA
        assert(false);
    #endif

    if (header.type == TcpHeader::PacketType::PURE_ACK
      && conn.in_fast_retransmit() == 0) // use only unambiguous acks
      congctrl.onACK(header.ack_num, header.receiver_timestamp);
  }

  congctrl.close();

  // Print stats
  cur_time = current_timestamp(start_time_point);

  double throughput = 1000.0 * conn.get_num_bytes_sent()
    / conn.get_data_transmit_duration();

  cout << "\n\nData Successfully Transmitted\n\tThroughput: "
    << throughput << " bytes/sec\n\tAverage Delay: "
    << conn.get_avg_rtt() / 1000.0 << " sec/packet\n";
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
