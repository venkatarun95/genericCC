#ifndef SEND_WINDOW_HH
#define SEND_WINDOW_HH

#include "configs.hh"
#include "events.hh"
#include "header-accessor.hh"
#include "process-header.hh"
#include "tcp-header.hh"
#include "window-blocks.hh"

// Decides which segment to transmit next given information about
// which bytes have been sent and acked. Uses Sack.
class SendWindowSack : public HeaderAccessor {
 public:
	// Takes event handler, 'pkt_recv', for when packet is
	// received. common_pipeline should pass through a congestion
	// controller which periodically fires packet pipes. This will fire
	// packet pipes on rtx timeout.
	SendWindowSack(PktRecvEvent& pkt_recv_event,
								 ProcessHeader rtx_pipeline);

  struct BlockData {
    // Last time this was sent
    Time last_sent_time;
    // Has this been acked yet?
    bool acked;
    // Number of times this segment has been sent
    int num_times_transmitted;

    bool operator==(const BlockData& other) const {
      return last_sent_time == other.last_sent_time 
        && acked == other.acked 
        && num_times_transmitted == other.num_times_transmitted;
    }
    bool operator!=(const BlockData& other) const {
      return !(*this == other);
    }
    // To enable debug printing in window-blocks for unit tests.
    operator int() const { return ((acked)?1:-1) * last_sent_time * 1e3; }
  };

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
	void RetransmitAlarmHandler();


 private:
	virtual void on_pkt_recv(TcpPacket headers);
  virtual void on_pkt_sent(TcpPacket headers, Time now);
  // Sets the variables `next_segment_start` and `next_segment_length`
  // with values for the next segment to transmit.
  void SetNextSegmentToTransmit(Time now);
	void SetRtxTime(Time now);

  // Read/write permissions for header fields
  AccessControlBits common_read = 
    CommonTcpHeader::A_SackBlocks 
    | CommonTcpHeader::A_WindowSize;
  AccessControlBits common_write = 0x0;
  AccessControlBits endpoint_read =
    EndpointHeader::A_RecvTime;
  AccessControlBits endpoint_write = 
    EndpointHeader::A_NextSendSeqNum
    | EndpointHeader::A_NextSendLength;

  static constexpr NumBytes max_segment_length = 1400;

	PktRecvEvent& pkt_recv_event;
	AlarmEvent rtx_alarm;
	ProcessHeader& rtx_pipeline;

  WindowBlocks<BlockData> window;
	// Time elapsed between last packet send time and its retransmission
  TimeDelta rtx_timeout;
	// Time at which to retransmit the oldest unacked packet.
	Time rtx_time;

  NumBytes next_segment_start;
  NumBytes next_segment_length;
};

#endif // SEND_WINDOW_HH
