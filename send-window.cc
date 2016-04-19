#include "send-window.hh"

#include <cassert>
#include <limits>

using namespace std;

SendWindowSack::SendWindowSack(PktRecvEvent& pkt_recv_event,
															 ProcessHeader common_pipeline)
	: pkt_recv_event(pkt_recv_event),
		rtx_alarm(),
		common_pipeline(common_pipeline),
		window(0),
		rtx_timeout(1),
		rtx_time(numeric_limits<Time>::max()),
		next_segment_start(0),
		next_segment_length(max_segment_length)
{}

void SendWindowSack::on_pkt_recv(TcpPacket pkt) {
	Time now = pkt.endpoint_header.get_recv_time(endpoint_read);
  for (int i = 0; i < CommonTcpHeader::num_sack_blocks; ++ i) {
		NumBytes start = pkt.common_header.get_sack_block(common_read, i, 0);
		NumBytes length = pkt.common_header.get_sack_block(common_read, i, 1);
    window.update_block(start,
                        length,
                        {0, true, 0});
  }
  
  SetNextSegmentToTransmit(now);
	SetRtxTime(now);

  pkt.endpoint_header.set_next_send_seq_num(endpoint_write, next_segment_start);
  pkt.endpoint_header.set_next_send_length(endpoint_write, next_segment_length);
}

void SendWindowSack::on_pkt_sent(TcpPacket pkt, Time now) {
  NumBytes start = pkt.common_header.get_seq_number(common_read);
	NumBytes length = pkt.common_header.get_data_len(common_read);

	auto prev_data = window.get_block(start);
	int num_times_transmitted = 1;
	if (prev_data.length > 0)
		num_times_transmitted = prev_data.data.num_times_transmitted + 1;

	window.update_block(start,
											length,
											BlockData {now, false, num_times_transmitted});
	SetNextSegmentToTransmit(now);
	SetRtxTime(now);
}

void SendWindowSack::RetransmitAlarmHandler() {
	TcpPacket pkt;
	rtx_pipeline.Process(pkt);
}

void SendWindowSack::SetNextSegmentToTransmit(Time now) {
  auto blocks = window.get_block_list();
	// So we don't retransmit the same packet again and again.
   int prev_num_transmitted = 0;

  for (auto x = blocks.end(); x != blocks.begin(); --x) {
    assert(now - x->data.last_sent_time >= 0);
		if (x->data.acked)
			continue;
		assert(prev_num_transmitted >= x->data.num_times_transmitted);
		// Retransmit
    if (prev_num_transmitted != x->data.num_times_transmitted
				&& now - x->data.last_sent_time >= rtx_timeout) {
      next_segment_start = x->start;
      next_segment_length = min(x->length, max_segment_length);
      assert(x->length % max_segment_length == 0);
      return;
    }
    prev_num_transmitted = x->data.num_times_transmitted;
  }

  // New data
  next_segment_start = blocks.back().start + blocks.back().length;
  next_segment_length = max_segment_length;
}

void SendWindowSack::SetRtxTime(Time now __attribute((unused))) {
	auto blocks = window.get_block_list();
	Time new_rtx_time = numeric_limits<Time>::max();
	for (auto x = blocks.end(); x != blocks.begin(); -- x) 
		new_rtx_time = min(new_rtx_time, x->data.last_sent_time + rtx_timeout);
	assert(new_rtx_time >= rtx_time);
	if (new_rtx_time != rtx_time) {
		rtx_alarm.SetAlarm(new_rtx_time);
		rtx_time = new_rtx_time;
	}
}
