#include <cstdint>

#include "configs.hh"

// Header which is assumed to be present in all TCP packets. Some
// versions of TCP or some special packets (such as SYN) may choose to
// append this with more header information.
struct CommonTcpHeader{
	// Sequence number of first byte in packet
	NumBytes seq_number;
	// Acknowledgement number
	NumBytes ack_number;
	// Maximum window size sender of packet is willing to receive in bytes
	NumBytes window_size;
	// Length of data in bytes
	NumBytes data_len;
	int16_t flags;

	static constexpr int8_t num_sack_blocks = 3;
	NumBytes sack_blocks[num_sack_blocks];

	// Number of optional headers present after this one.
	int8_t num_optional_headers;
};

// Base struct for all optional headers
struct OptionalHeaderBase {
	const int16_t magic_num;
};

// Basic flags to use, others are configurable
constexpr int tcp_header_flag_syn = 0x1;
constexpr int tcp_header_flag_ack = 0x2;
constexpr int tcp_header_flag_fin = 0x4;
