#include <vector>

struct TCPHeader{
	// Seq_num of data packet. Valid only for data packets
	int seq_num;
	// Seq num of expected packet (in case of an ACK packet). Valid only
	// for ACK packets.
	int ack_num;
	int flow_id;
	int src_id;
	// Time when the packet was sent (in ms).
	double sender_timestamp;
	// Time when the packet to which this is an ACK was received (in ms).
	// The clocks for the two timestamps need not be synchronized.
	double receiver_timestamp;
};


class TcpConnection {
	// Implements a sliding window. Of constant size equal to receive
	// window size, has true iff that packet has been acked.
	vector<bool> window;
	// position where window begins in `window`
	unsigned window_pos;
	unsigned expected_seq_no;

public:
	TcpConnection(unsigned rcv_window_size=65536)
	:	window(rcv_window_size, false),
		window_pos(0),
		expected_seq_no(0)
	{}

	static void interactive_test();

	int register_received_packet(TCPHeader pkt);
};
