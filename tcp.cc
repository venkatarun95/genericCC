#include <cassert>
#include <cmath>
#include <iostream>

#include "tcp.hh"

using namespace std;

string TcpHeader::tcpdump() {
  string res = to_string(type) + " " + to_string(seq_num) + " " + \
    to_string(ack_num) + " " + to_string(size);
  return res;
}

void TcpConnection::register_data_pkt(unsigned seq_num) {
	assert(rcv_window[rcv_window_pos] == false);
	assert(seq_num - expected_seq_no < rcv_window.size());
  assert(!ack_pending);
	if (seq_num == expected_seq_no) {
		rcv_window_pos = (rcv_window_pos + 1) % rcv_window.size();
		expected_seq_no ++;
		while (rcv_window[rcv_window_pos] == true) {
			rcv_window[rcv_window_pos] = false;
			rcv_window_pos = (rcv_window_pos + 1) % rcv_window.size();
			expected_seq_no ++;
		}
	}
	else {
		rcv_window[(rcv_window_pos + seq_num - expected_seq_no)%rcv_window.size()] \
      = true;
	}
  ack_pending = true;
}

void TcpConnection::register_ack(unsigned ack_num) {
  assert(ack_num > last_acked_pkt);
  if (ack_num == last_acked_pkt + 1) {
    num_dupacks ++;
    if (num_dupacks >= 3)
    handle_dupack();
    return;
  }

  num_dupacks = 0;
  assert(ack_num == last_acked_pkt + 2); // SACK not yet supported
  last_acked_pkt = ack_num - 1;
  assert (last_acked_pkt <= last_transmitted_pkt);
  if (want_to_close && last_acked_pkt == last_transmitted_pkt) {
    want_to_close = false;
    state = ConnState::FIN_WAIT;
    est_term_pkt_sent = false;
  }
}

void TcpConnection::handle_dupack() {
  cout << "Warning: Dupack handling not yet supported" << endl;
}

void TcpConnection::register_received_packet(TcpHeader pkt) {
  assert(pkt.valid);
  echo_timestamp = pkt.sender_timestamp;
  switch(state) {
    case ConnState::BEGIN:
      assert(pkt.seq_num > 0 && pkt.ack_num == 0 && pkt.size == 0);
      assert(pkt.type == TcpHeader::PacketType::SYN);
      rcv_window.resize(min(pkt.seq_num, (unsigned)rcv_window.size()), false);
      state = ConnState::SYN_RECEIVED;
      est_term_pkt_sent = false;
    break;
    case ConnState::SYN_SENT:
      state = ConnState::ESTABLISHED;
      est_term_pkt_sent = false; // this causes the third SYN to be sent.
    break;
    case ConnState::SYN_RECEIVED:
      assert(pkt.seq_num == 1 && pkt.ack_num == 0);
      assert(pkt.type == TcpHeader::PacketType::SYN);
      pkt.type = TcpHeader::PacketType::PURE_DATA;
      state = ConnState::ESTABLISHED;
      // Note no break. This is a data packet too.
    case ConnState::ESTABLISHED:
      if (pkt.type == TcpHeader::PacketType::PURE_DATA){
        assert(pkt.size > 0);
        register_data_pkt(pkt.seq_num);
      }
      else if (pkt.type == TcpHeader::PacketType::PURE_ACK)
        register_ack(pkt.ack_num);
      else if (pkt.type == TcpHeader::PacketType::FIN) {
        assert (pkt.seq_num == 1 && pkt.ack_num == 0 && pkt.size == 0);
        state = ConnState::CLOSE_WAIT;
        est_term_pkt_sent = false;
      }
      else
        assert (false); // This packet was not expected in ESTABLISHED
    break;
    case ConnState::FIN_WAIT:
      assert(pkt.seq_num == 2 && pkt.type == TcpHeader::PacketType::FIN);
      assert(pkt.ack_num == 0 && pkt.size == 0);
      state = ConnState::CLOSED;
      est_term_pkt_sent = true;
    break;
    case ConnState::CLOSED:
      cerr << "Received packet after connection is closed - host: "
        << host_id << " flow: " << flow_id << "\n\t" << pkt.tcpdump();
      break;
    default:
      assert(false); // Unrecognized state to get a packet in
  }
}

TcpHeader TcpConnection::get_next_pkt(double cur_time, unsigned size) {
  assert(!(size > 0 && ack_pending)); // piggybacking not yet supported
  TcpHeader header;
  header.flow_id = flow_id;
  header.src_id = host_id;
  if (last_transmitted_pkt - last_acked_pkt == snd_window_size) {
    header.valid = false;
    return header;
  }
  switch (state) {
    case ConnState::ESTABLISHED:
      assert(last_transmitted_pkt - last_acked_pkt < snd_window_size);
      if (size > 0) {
        if (est_term_pkt_sent == false) { // Third SYN packet
          header.type = TcpHeader::PacketType::SYN;
          header.seq_num = 1;
          last_transmitted_pkt = 1;
          est_term_pkt_sent = true;
        }
        else {
          header.type = TcpHeader::PacketType::PURE_DATA;
          if (retransmission_pending)
            header.seq_num = last_transmitted_pkt;
          else {
            header.seq_num = last_transmitted_pkt + 1;
            last_transmitted_pkt ++;
          }
        }
        header.ack_num = 0;
        header.size = size;
        header.sender_timestamp = cur_time;
        header.receiver_timestamp = 0;
        header.valid = true;
      }
      else if (ack_pending) {
        ack_pending = false;
        header.type = TcpHeader::PacketType::PURE_ACK;
        header.seq_num = 0;
        header.ack_num = expected_seq_no;
        header.size = 0;
        header.sender_timestamp = echo_timestamp;
        header.receiver_timestamp = cur_time;
        header.valid = true;
      }
      else
        header.valid = false;
    break;

    case ConnState::BEGIN:
      // Cannot transmit before establishing connection
      // It is possible the receiver might try to transmit prematurely
      header.valid = false;
    break;

    case ConnState::SYN_SENT:
      // First syn packet. Third syn packet is sent in ESTABLISHED
      // state when est_term_pkt_sent is false
      if (est_term_pkt_sent) {
        header.valid = false;
        break;
      }
      est_term_pkt_sent = true;
      header.type = TcpHeader::PacketType::SYN;
      header.seq_num = snd_window_size;
      header.ack_num = 0;
      header.size = 0;
      header.sender_timestamp = cur_time;
      header.receiver_timestamp = 0;
      header.valid = true;
    break;

    case ConnState::SYN_RECEIVED:
      if (est_term_pkt_sent) {
        header.valid = false;
        break;
      }
      est_term_pkt_sent = true;
      header.type = TcpHeader::PacketType::SYN;
      // Min operation is already performed on recipt of first SYN pkt.
      header.seq_num = rcv_window.size();
      header.ack_num = 0;
      header.size = 0;
      header.sender_timestamp = echo_timestamp;
      header.receiver_timestamp = cur_time;
      header.valid = true;
    break;

    case ConnState::FIN_WAIT:
      if (est_term_pkt_sent) {
        header.valid = false;
        break;
      }
      est_term_pkt_sent = true;
      header.type = TcpHeader::PacketType::FIN;
      header.seq_num = 1;
      header.ack_num = 0;
      header.size = 0;
      header.sender_timestamp = cur_time;
      header.receiver_timestamp = 0;
      header.valid = true;
    break;

    case ConnState::CLOSE_WAIT:
      // Second FIN packet
      assert(est_term_pkt_sent == false);
      est_term_pkt_sent = true;
      header.type = TcpHeader::PacketType::FIN;
      header.seq_num = 2;
      header.ack_num = 0;
      header.size = 0;
      header.sender_timestamp = 0;
      header.valid = true;
      // only place where state change happens in this function (while
      // sending a packet)
      state = ConnState::CLOSED;
    break;

    case ConnState::CLOSED:
      assert (false);
    break;
    default:
      assert(false);
  }
  return header;
}

bool TcpConnection::transmit_immediately() {
  if (ack_pending || retransmission_pending || !est_term_pkt_sent)
    return true;
  return false;
}

void TcpConnection::establish_connection() {
  state = TcpConnection::ConnState::SYN_SENT;
  est_term_pkt_sent = false;
}

void TcpConnection::close_connection() {
  if (last_transmitted_pkt == last_acked_pkt) {
    state = ConnState::FIN_WAIT;
    est_term_pkt_sent = false;
  }
  else {
    assert(last_acked_pkt < last_transmitted_pkt);
    want_to_close = true;
  }
}

void TcpConnection::interactive_test() {
	int window_size;
	cout << "Enter window size: ";
	cin >> window_size;
  TcpConnection snd(0, 1, window_size);
  TcpConnection rcv(1, 0, window_size);

  int data_remaining = 15001;
  snd.establish_connection();
  while (snd.get_state() != ConnState::CLOSED ||
  snd.get_state() != ConnState::CLOSED) {
    TcpHeader header = snd.get_next_pkt(0.0, min(1500, data_remaining));
    if (header.valid) {
      data_remaining -= header.size;
      if (data_remaining <= 0)
        snd.close_connection();
      cout << "Snd: " << rcv.get_state() << " " << header.tcpdump() << endl;
      rcv.register_received_packet(header);
    }
    else
      cout << "Snd: no packet " << rcv.get_state() << " " << endl;

    header = rcv.get_next_pkt(0.0, 0);
    if (header.valid) {
      cout << "Rcv: " << rcv.get_state() << " " << header.tcpdump() << endl;
      snd.register_received_packet(header);
    }
    else
      cout << "Rcv: no packet" << rcv.get_state() << " " << endl;
  }
}
