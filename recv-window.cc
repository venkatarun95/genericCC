#include "recv-window.hh"

RecvWindowSack::RecvWindowSack()
	: window(0)
{}

void RecvWindowSack::AccessHeader(TcpPacket& pkt) {
	if (pkt.endpoint_header.get_type(endpoint_read) == 
			EndpointHeader::HdrType::INCOMING_PACKET) {
		// Update recv window.
		window.update_block(pkt.common_header.get_seq_number(common_read),
												pkt.common_header.get_data_len(common_read),
												true);
	}
	else if (pkt.endpoint_header.get_type(endpoint_read) ==
					 EndpointHeader::HdrType::OUTGOING_PACKET) {
		// Construct sack block.
		int i = 0;
		for (const auto& x : window.get_block_list()) {
			pkt.common_header.set_sack_block(common_write, x.start, i, 0);
			pkt.common_header.set_sack_block(common_write, x.length, i, 1);
			++ i;
			if (i == 3) break;
		}
	}
}
