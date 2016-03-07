#ifndef TCP_HEADER_HH
#define TCP_HEADER_HH

#include "configs.hh"

#include <cassert>
#include <cstdint>
#include <unordered_map>

// Base class for all headers
struct HeaderBase {
  virtual ~HeaderBase();
};

// Header which is assumed to be present in all TCP packets. Some
// versions of TCP or some special packets (such as SYN) may choose to
// append this with more header information.
struct CommonTcpHeader : public HeaderBase{
	// Sequence number of first byte in packet
	NumBytes seq_number;
	// Acknowledgement number
	//// NumBytes ack_number;
	// Maximum window size sender of packet is willing to receive in bytes
	NumBytes window_size;
	// Length of data in bytes
	NumBytes data_len;
	int16_t flags;

	static constexpr int8_t num_sack_blocks = 3;
  // In each block, the first element is the starting sequence number
  // and the second element is the length of the block.
	NumBytes sack_blocks[num_sack_blocks][2];

	// Number of optional headers present after this one.
	int8_t num_optional_headers;

  // Basic flags to use, others are configurable
  static constexpr int flag_syn = 0x1;
  static constexpr int flag_ack = 0x2;
  static constexpr int flag_fin = 0x4;

	// Access control for header fields
	static constexpr AccessControlBits A_SeqNumber = 0x01;
	static constexpr AccessControlBits A_WindowSize = 0x02;
	static constexpr AccessControlBits A_DataLen = 0x04;
	static constexpr AccessControlBits A_Flags = 0x08;
	static constexpr AccessControlBits A_SackBlocks = 0x10;

	// These access functions should be optimized out as the values in
	// the assert calls will be known at compile time.
	NumBytes get_seq_number(AccessControlBits read_permissions) const {
		assert(A_SeqNumber & read_permissions);
		return seq_number;
	}
	NumBytes get_window_size(AccessControlBits read_permissions) const {
		assert(A_WindowSize & read_permissions);
		return window_size;
	}
	NumBytes get_data_len(AccessControlBits read_permissions) const {
		assert(A_DataLen & read_permissions);
		return data_len;
	}
	int16_t get_flags(AccessControlBits read_permissions) const {
		assert(A_Flags & read_permissions);
		return flags;
	}
	NumBytes get_sack_block(AccessControlBits read_permissions, int i, int x) const {
		assert(A_SackBlocks & read_permissions);
		assert(i < num_sack_blocks && (x == 0 || x == 1));
		return sack_blocks[i][x];
	}

	void set_seq_number(AccessControlBits write_permissions, NumBytes s_seq_number) {
		assert(A_SeqNumber & write_permissions);
		seq_number = s_seq_number;
	}
	void set_window_size(AccessControlBits write_permissions, NumBytes s_window_size) {
		assert(A_WindowSize & write_permissions);
		window_size = s_window_size;
	}
	void set_data_len(AccessControlBits write_permissions, NumBytes s_data_len) {
		assert(A_DataLen & write_permissions);
		data_len = s_data_len;
	}
	void set_flags(AccessControlBits write_permissions, NumBytes s_flags) {
		assert(A_Flags & write_permissions);
		flags = s_flags;
	}
	void set_sack_block(AccessControlBits write_permissions, NumBytes data, int i, int x) {
		assert(A_SackBlocks & write_permissions);
    assert(i < num_sack_blocks && (x == 0 || x == 1));
		sack_blocks[i][x] = data;
	}
};

// This is for communication between the various header accessors and
// the code that actually sends/receives data. It is never actually
// directly sent 'on the wire' and always stays within a single
// endpoint.
struct EndpointHeader : public HeaderBase {
  // The time at which the packet was received.
  Time recv_time;
  // The length and starting sequence number of the next packet to be
  // sent.
  NumBytes next_send_seq_num;
  NumBytes next_send_length;
  
  static constexpr AccessControlBits A_RecvTime = 0x01;
  static constexpr AccessControlBits A_NextSendSeqNum = 0x02;
  static constexpr AccessControlBits A_NextSendLength = 0x04;

  // These access functions should be optimized out as the values in
	// the assert calls will be known at compile time.
  Time get_recv_time(AccessControlBits read_permissions) const {
    assert(A_RecvTime & read_permissions);
    return recv_time;
  }
  NumBytes get_next_send_seq_num(AccessControlBits read_permissions) const {
    assert(A_NextSendSeqNum & read_permissions);
    return next_send_seq_num;
  }
  NumBytes get_next_send_length(AccessControlBits read_permissions) const {
    assert(A_NextSendLength & read_permissions);
    return next_send_length;
  }

  void set_recv_time(AccessControlBits write_permissions, Time s_recv_time) {
    assert(A_RecvTime & write_permissions);
    recv_time = s_recv_time;
  }
  void set_next_send_seq_num(AccessControlBits write_permissions, NumBytes s_next_send_seq_num) {
    assert(A_NextSendSeqNum & write_permissions);
    next_send_seq_num = s_next_send_seq_num;
  }
  void set_next_send_length(AccessControlBits write_permissions, NumBytes s_next_send_length) {
    assert(A_NextSendLength & write_permissions);
    next_send_length = s_next_send_length;
  }
};

// Base struct for all optional headers
struct OptionalHeaderBase : public HeaderBase {
	const HeaderMagicNum magic_num;
  OptionalHeaderBase() : magic_num(-1) {}
};


// Structure containing all headers representing a packet.
struct TcpPacket {
  CommonTcpHeader common_header;
  EndpointHeader endpoint_header;
  std::unordered_map<HeaderMagicNum, HeaderBase> optional_headers;
};

#endif
