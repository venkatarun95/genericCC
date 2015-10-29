#include <cassert>
#include <cmath>
#include <iostream>

#include "tcp.hh"

using namespace std;

string TcpHeader::tcpdump() const {
  string type_str = "ERR";
  switch (type) {
    case PacketType::PURE_ACK: type_str = "ACK"; break;
    case PacketType::PURE_DATA: type_str = "DAT"; break;
    case PacketType::SYN: type_str = "SYN"; break;
    case PacketType::FIN: type_str = "FIN"; break;
  }
  string res = type_str + " seq " + to_string(seq_num) + ", ack " + \
    to_string(ack_num) + ", " + to_string(size) + " bytes";
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

void TcpConnection::register_ack(const TcpHeader& pkt) {
  assert(pkt.type != TcpHeader::PacketType::PURE_DATA);
  if (pkt.type == TcpHeader::PacketType::PURE_ACK) {
    unsigned ack_num = pkt.ack_num;
    assert(ack_num > last_acked_pkt);
    // Check for dupack
    if (ack_num == last_acked_pkt + 1) {
      num_dupacks ++;
      if (num_dupacks >= 3)
        handle_dupack();
      return;
    }
    else {
      num_dupacks = 0;
      assert (last_acked_pkt <= last_transmitted_pkt);

      // If we want to close, check if all outstanding pkts have been acked.
      if (want_to_close && last_acked_pkt == last_transmitted_pkt) {
        want_to_close = false;
        state = ConnState::FIN_WAIT;
        est_term_pkt_sent = false;
      }
    }
  }
  if (num_dupacks > 0)
    return;

  // Find the packet in the snd_window.
  // We trust that this loop won't run too many iterations.
  unsigned i = snd_window_pos;
  while ( true ) {
    if (snd_window[i].second.type == TcpHeader::PacketType::PURE_DATA
      && pkt.type == TcpHeader::PacketType::PURE_ACK) {
      if (snd_window[i].second.seq_num == pkt.ack_num - 1) {
        break;
      }
    }
    else if (snd_window[i].second.type == TcpHeader::PacketType::SYN
      && snd_window[i].second.seq_num == 1) // ack to third SYN packet
      break;
    else if (snd_window[i].second.type == pkt.type) // for SYN and FIN pkts
        break;
    i = (i + 1) % snd_window.size();
    // Assert: Unknown ack.
    assert(((i - snd_window_pos) % snd_window.size()) < snd_window_size);
  }
  // This is not a dupack, as they were caught earlier in the function.
  assert(snd_window[i].first == false);
  snd_window[i].first = true;
  while (snd_window[snd_window_pos].first == true && snd_window_size > 0) {
    if (snd_window[snd_window_pos].second.size > 0) {
      assert(snd_window[snd_window_pos].second.seq_num == last_acked_pkt + 1);
      last_acked_pkt = snd_window[snd_window_pos].second.seq_num;
    }
    snd_window_size --;
    snd_window_pos = (snd_window_pos + 1) % snd_window.size();
  }
}

void TcpConnection::register_sent_packet(const TcpHeader& pkt) {
  assert(snd_window_size < snd_window.size());
  assert(pkt.valid);
  snd_window[(snd_window_pos + snd_window_size) % snd_window.size()] =
    make_pair(false, pkt);
  snd_window_size ++;
}

void TcpConnection::handle_dupack() {
  retransmission_pending = true;
}

void TcpConnection::track_stats(const TcpHeader& pkt, double cur_time, bool ack) {
  if (ack) {
    assert(pkt.type == TcpHeader::PacketType::PURE_ACK);
    assert(cur_time > data_transmission_end_time);
    if (smooth_rtt == -1.0) {
      smooth_rtt = cur_time - pkt.sender_timestamp;
    }
    smooth_rtt = alpha_smooth_rtt * (cur_time - pkt.sender_timestamp) +
      (1 - alpha_smooth_rtt) * smooth_rtt;
    // Use cur_time and not receiver_timestamp because the clocks may
    // not be in sync in case of real world usage.
    sum_rtt += cur_time - pkt.sender_timestamp;
    num_bytes_acked += pkt.size; // Assuming it is not a dupack
    num_pkts_acked += 1;
    data_transmission_end_time = cur_time;
  }
  else {
    if (!(pkt.type == TcpHeader::PacketType::PURE_DATA
      || (pkt.type == TcpHeader::PacketType::SYN && pkt.seq_num == 1)))
      return;
    assert(pkt.size > 0);
    if (data_transmission_start_time == -1.0) {
      data_transmission_start_time = cur_time;
    }
    num_bytes_sent += pkt.size;
    num_pkts_sent += 1;
  }
}

void TcpConnection::register_received_packet(const TcpHeader& pkt, double cur_time) {
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
      register_ack(pkt);
      snd_window.resize(std::min((unsigned)snd_window.size(), pkt.seq_num),
        make_pair(false, TcpHeader()));
      // receiver should have already computed the correct size
      assert(snd_window.size() == pkt.seq_num);
      state = ConnState::ESTABLISHED;
      est_term_pkt_sent = false; // this causes the third SYN to be sent.
    break;
    case ConnState::SYN_RECEIVED:
      assert(pkt.seq_num == 1 && pkt.ack_num == 0);
      assert(pkt.type == TcpHeader::PacketType::SYN);
      state = ConnState::ESTABLISHED;
      // Note no break. This is a data packet too.
    case ConnState::ESTABLISHED:
      if (pkt.type == TcpHeader::PacketType::PURE_DATA
        || pkt.type == TcpHeader::PacketType::SYN) {
        assert(pkt.size > 0);
        register_data_pkt(pkt.seq_num);
      }
      else if (pkt.type == TcpHeader::PacketType::PURE_ACK) {
        track_stats(pkt, cur_time, true);
        register_ack(pkt);
      }
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
      register_ack(pkt);
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
  if (last_transmitted_pkt - last_acked_pkt == snd_window.size()) {
    header.valid = false;
    return header;
  }
  switch (state) {
    case ConnState::ESTABLISHED:
      assert(last_transmitted_pkt - last_acked_pkt < snd_window.size());
      if (size > 0) {
        if (est_term_pkt_sent == false) { // Third SYN packet
          header.type = TcpHeader::PacketType::SYN;
          header.seq_num = 1;
          last_transmitted_pkt = 1;
          est_term_pkt_sent = true;
          header.ack_num = 0;
          header.size = size;
          header.sender_timestamp = cur_time;
          header.receiver_timestamp = 0;
          header.valid = true;
          track_stats(header, cur_time, false);
          register_sent_packet(header);
        }
        else {
          header.type = TcpHeader::PacketType::PURE_DATA;
          if (retransmission_pending) {
            assert(!snd_window[snd_window_pos].first);
            header = snd_window[snd_window_pos].second;
            retransmission_pending = false;
          }
          else if (want_to_close){
            header.valid = false;
          }
          else {
            header.seq_num = last_transmitted_pkt + 1;
            last_transmitted_pkt ++;
            header.ack_num = 0;
            header.size = size;
            header.sender_timestamp = cur_time;
            header.receiver_timestamp = 0;
            header.valid = true;
            track_stats(header, cur_time, false);
            register_sent_packet(header);
          }
        }
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
      header.seq_num = snd_window.size();
      header.ack_num = 0;
      header.size = 0;
      header.sender_timestamp = cur_time;
      header.receiver_timestamp = 0;
      header.valid = true;
      register_sent_packet(header);
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
      register_sent_packet(header);
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

bool TcpConnection::transmit_immediately() const {
  if (ack_pending || retransmission_pending || !est_term_pkt_sent)
    return true;
  return false;
}

void TcpConnection::establish_connection() {
  state = TcpConnection::ConnState::SYN_SENT;
  est_term_pkt_sent = false;
}

void TcpConnection::close_connection() {
  if (state == ConnState::BEGIN || state == ConnState::SYN_SENT
    || state == ConnState::SYN_RECEIVED) {
    cerr << "Warning: Trying to close connection that has not been\
    established yet. Ignoring close instruction." << endl;
    return;
  }
  if (state != ConnState::ESTABLISHED) {
    return; // Already closing
  }
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
  // bool ans;
	cout << "Enter window size: ";
	cin >> window_size;
  TcpConnection snd(0, 1, window_size);
  TcpConnection rcv(1, 0, window_size);

  int data_remaining = 15001;
  double cur_time = 0.0;

  snd.establish_connection();
  while (snd.get_state() != ConnState::CLOSED ||
  snd.get_state() != ConnState::CLOSED) {
    if (!snd.transmit_immediately())
      cur_time += 0.1;
    TcpHeader header = snd.get_next_pkt(cur_time, min(1500, data_remaining));
    if (header.valid) {
      data_remaining -= header.size;
      if (data_remaining <= 0)
        snd.close_connection();
      cout << "Snd (" << cur_time << " ms): " << rcv.get_state() << " "
        << header.tcpdump() << endl;

      // cin >> ans; // ask user if packet should be forwarded
      // if (ans)
      rcv.register_received_packet(header, cur_time);
    }
    else
      cout << "Snd: no packet " << rcv.get_state() << " " << endl;

    if (!rcv.transmit_immediately())
      cur_time += 0.1;
    header = rcv.get_next_pkt(cur_time, 0);
    if (header.valid) {
      cout << "Rcv (" << cur_time << " ms): " << rcv.get_state() << " "
        << header.tcpdump() << endl;
      // cin >> ans; // ask user if packet should be forwarded
      // if (ans)
      snd.register_received_packet(header, cur_time);
    }
    else
      cout << "Rcv: no packet" << rcv.get_state() << " " << endl;
  }
}
