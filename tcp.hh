#include <tuple>
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
  std::string tcpdump() const;
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

 public:
  enum ConnState { BEGIN, SYN_SENT, SYN_RECEIVED, ESTABLISHED, FIN_WAIT, \
    CLOSE_WAIT, CLOSED };

 private:
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
  // Slight misnomer. Last acked pkt such that all previous pkts have
  // also been acked. In seq num.
  unsigned last_acked_pkt;
  // Sender window. bool stores whether the packet has been acked.
  std::vector< std::pair<bool, TcpHeader> > snd_window;
  // Position of oldest packet in snd_window;
  unsigned snd_window_pos;
  // Number of packets in snd_window
  unsigned snd_window_size;
  // Whether or not a retransmission is pending. This should be
  // irrespective of congestion window or pacing and hence elicits a
  // true from transmit_immediately.
  bool retransmission_pending;
  // Set when close_connection is called and before all pkts are acked.
  bool want_to_close;


  // For maintaining statistics (at sender)

  const double alpha_smooth_rtt = 1.0/16.0;
  double smooth_rtt;
  // For avg. RTT
  double sum_rtt;
  int num_bytes_sent;
  int num_pkts_sent;
  int num_bytes_acked;
  int num_pkts_acked;
  // Timestamp when transmission started (ie. 3rd SYN pkt).
  double data_transmission_start_time;
  // Always stores time when last ack was received
  double data_transmission_end_time;


  // Called at the sender's side
  void handle_dupack();
  // Called at the sender's side
  void register_ack(const TcpHeader& pkt);
  // Called at the receiver's side
  void register_data_pkt(unsigned seq_num);
  // Called at the sender's side
  void register_sent_packet(const TcpHeader& pkt);
  // Called when a new ack is received before it is removed from
  // snd_window with `ack=true`.
  // Called before a data containing packet is sent (in get_next_pkt)
  // is sent with `ack=false`.
  void track_stats(const TcpHeader& pkt, double cur_time, bool ack);

 public:
  TcpConnection(unsigned host_id=0, unsigned flow_id=0, unsigned window_size=65536)
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
    snd_window(window_size, std::make_pair(false, TcpHeader())),
    snd_window_pos(0),
    snd_window_size(0),
    retransmission_pending(false),
    want_to_close(false),
    smooth_rtt(-1.0),
    sum_rtt(0.0),
    num_bytes_sent(0),
    num_pkts_sent(0),
    num_bytes_acked(0),
    num_pkts_acked(0),
    data_transmission_start_time(-1.0),
    data_transmission_end_time(-1.0)
	{}

	static void interactive_test();

  // Should be called as soon as a packet is received.
	void register_received_packet(const TcpHeader& pkt, double cur_time);
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
  bool transmit_immediately() const;
  // Actively established connection by making the right packet
  // available at 'get_next_pkt'.
  void establish_connection();
  // Actively terminates connection by making the right packet
  // available at 'get_next_pkt'. Should be called only by sender (this
  // is the reason bidirectional data transfer does not work yet)
  void close_connection();

  // Getter functions
  ConnState get_state() const {return state;}
  unsigned get_num_bytes_sent() const { return num_bytes_sent; }
  unsigned get_num_bytes_acked() const { return num_bytes_acked; }
  // Useful for congestion control
  unsigned get_snd_window_size() const {return snd_window_size; }
  double get_data_transmit_duration() const
    { return data_transmission_end_time - data_transmission_start_time; }
  double get_avg_rtt() const { return sum_rtt / num_pkts_sent; }
  unsigned get_num_dupacks() const { return num_dupacks; }
};
