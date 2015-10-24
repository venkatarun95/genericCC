#include <vector>

struct TcpHeader{

  enum PacketType { PURE_ACK, PURE_DATA, SYN, FIN };
  PacketType type;
	// Seq_num of data packet. Valid only for PURE_DATA packets.
	unsigned seq_num;
	// Seq num of expected packet. Valid only for PURE_ACK packets.
	unsigned ack_num;
	unsigned flow_id;
	unsigned src_id;
  // Size of payload in bytes
  unsigned size;
	// Time when the packet was sent (in ms).
	double sender_timestamp;
	// Time when the packet to which this is an ACK was received (in ms).
	// The clocks for the two timestamps need not be synchronized.
	double receiver_timestamp;
  // Used by 'get_next_pkt'. If no packet needs to be transmitted, this
  // is set to false
  bool valid;

  // Get a string representing the packet.
  // TODO(venkat) : make it the same as what is printed by tcpdump
  std::string tcpdump();
};


class TcpConnection {
  // PROTOCOL SPECIFICATION:-
  //
  //
  // Connection Establishment and Termination Protocol
  //
  // Starts at BEGIN. When syn is sent, client moves to SYN_SENT on
  // recipt of which, server moves to SYN_RECEIVED. Server sends a pkt
  // back to client which moves it to ESTABLISHED. The client sends a
  // SYN packet with data, which causes the server to be ESTABLISHEd.
  //
  // One of them sends FIN to the other and moves to FIN_WAIT till
  // FIN packet is acked after which is goes to CLOSED. The other
  // end moves to CLOSE_WAIT upon recipt of first FIN packet and after
  // sending the ack, moves to CLOSED.
  // TODO(venkat): If this ack is lost, it is possible that the sender
  // remains open. Ideally this should keep sending the FIN packets
  // until some timeout after which it should CLOSE. This is not yet
  // implemented.
  //
  // Packet Formats:
  // If packet type is SYN or FIN, meaning of seq_num and ack_num
  // changes. In the first two SYN packets, seq_num=window_size. In the
  // third SYN packet, seq_num=1.
  // In the first and second FIN packet, seq_num=1 and 2 respectively.
  // ack_num remains 0 in all these packets.
  //
  //
  // NOTES:
  // - Sequence nums start from 1 (so that 0 can be used as sentinel).
  // - Interface is mainly through 'register_received_packet()'
  //  and 'get_next_pkt()'. These should be called whenever a packet is
  //  received or to be sent, details of connection establishment,
  //  retransmission, windowing etc. will be managed by these functions.
  //  'transmit_immediately' also helps.
  // - When close_connection() is called, 'want_to_close' is set to
  //  true and we wait until all packets are acked before sending the
  //  FIN packet and moving to FIN_WAIT state. Since one-sided closed
  //  connections are not supported, close_connection() must always be
  //  sent from the sender side. This is also the reason why
  //  bidirectional tcp is not yet supported. It should be an easy
  //  addon though.

  // Common between sender and receiver

  enum ConnState { BEGIN, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT, \
    CLOSE_WAIT, CLOSED };
  ConnState state;
  unsigned num_dupacks;
  // Whether or not there is an ack pending that needs to be sent
  bool ack_pending;
  // Used for filling sender_id and flow_id in header
  unsigned host_id;
  unsigned flow_id;
  // Timestamp in last data packet received, so be echoed back.
  double echo_timestamp;
  // In connection establishment and termination, the state is changed
  // before any packets are sent. This variable tracks whether that
  // packet has been sent.
  bool est_term_pkt_sent;

  // Receiver's side of things

	// Implements a sliding window. Of constant size equal to receive
	// window size, has true iff that packet has been acked.
	std::vector<bool> rcv_window;
	// Position where receive window begins in `rcv_window`
	unsigned rcv_window_pos;
	unsigned expected_seq_no;

  // Sender's side of things

  // In seq num.
  unsigned last_transmitted_pkt;
  // In seq num. Last packet acked by the other side
  unsigned last_acked_pkt;
  // In pkts. Window size for flow control.
  unsigned snd_window_size;
  // Whether or not a retransmission is pending. This should be
  // irrespective of congestion window or pacing and hence elicits a
  // true from transmit_immediately.
  bool retransmission_pending;
  // Set when close_connection is called and before all pkts are acked.
  bool want_to_close;


  void handle_dupack();

  void register_ack(unsigned ack_num);
  void register_data_pkt(unsigned seq_num);

public:
	TcpConnection(unsigned host_id, unsigned flow_id, unsigned window_size=65536)
	:	state(ConnState::BEGIN),
    num_dupacks(0),
    ack_pending(false),
    host_id(host_id),
    flow_id(flow_id),
    echo_timestamp(),
    est_term_pkt_sent(true),
    rcv_window(window_size, false),
		rcv_window_pos(0),
		expected_seq_no(1),
    last_transmitted_pkt(0),
    last_acked_pkt(0),
    snd_window_size(window_size),
    retransmission_pending(false),
    want_to_close(false)
	{}

	static void interactive_test();

  // Should be called as soon as a packet is received.
	void register_received_packet(TcpHeader pkt);
  // Returns header for next packet to transmit. If no transmission is
  // required, 'valid' bit in header is set to false. 'size' gives the
  // number of bytes of data to transmit with the packet. 'cur_time'
  // gives the current timestamp in ms.
  // This function should be called only when we are ready to transmit
  // a packet if required, as if it returns a packet with valid bit set,
  // it assumes that that packet has been sent. Further the caller must
  // also check the 'size' in the returned header. It may be less than
  // the given size (typically 0).
  TcpHeader get_next_pkt(double cur_time, unsigned size=0);
  // Returns true if a packet needs to be transmitted immediately or
  // if we can wait for packet pacing.
  bool transmit_immediately();
  // Actively established connection by making the right packet
  // available at 'get_next_pkt'.
  void establish_connection();
  // Actively terminates connection by making the right packet
  // available at 'get_next_pkt'.
  void close_connection();

  ConnState get_state() {return state;}
};
