#ifndef RECV_WINDOW_HH
#define RECV_WINDOW_HH

#include "configs.hh"
#include "events.hh"
#include "header-accessor.hh"
#include "tcp-header.hh"
#include "window-blocks.hh"

// Keeps track of received bytes so that it can construct and send
// acknowledgements (with SACK). It should belong to pipelines such
// that it accesses each incoming packet (so that it can note down
// what bytes have been received) and outgoing packet (so that it can
// construct the Sack block).
class RecvWindowSack : public HeaderAccessor {
 public:
	RecvWindowSack();

  virtual std::vector< std::pair<HeaderMagicNum, AccessControlBits> >
  get_read_permissions() const {
    return {std::make_pair(endpoint_header_magic_num, endpoint_read),
        std::make_pair(common_header_magic_num, common_read)};
  }
  virtual std::vector< std::pair<HeaderMagicNum, AccessControlBits> >
  get_write_permissions() const {
    return {std::make_pair(endpoint_header_magic_num, endpoint_write),
        std::make_pair(common_header_magic_num, common_write)};
  }

	virtual void AccessHeader(TcpPacket&) override;

 private:
	// Read/write permissions for header fields
  AccessControlBits common_read = 
    CommonTcpHeader::A_SeqNumber
		| CommonTcpHeader::A_DataLen;
  AccessControlBits common_write = CommonTcpHeader::A_SackBlocks;
  AccessControlBits endpoint_read = EndpointHeader::A_Type;
  AccessControlBits endpoint_write = 0x0;

	WindowBlocks<bool> window;
};

#endif // RECV_WINDOW_HH
