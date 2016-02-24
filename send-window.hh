#ifndef SEND_WINDOW_HH
#define SEND_WINDOW_HH

#include "configs.hh"
#include "tcp-header.hh"

#include <iostream>
#include <limits>
#include <list>
#include <queue>

// Support class to maintain state about bytes in a window.
//
// Note that wrapping around of sequence numbers is not yet
// supported. It is assumed that newer bytes have higher sequence
// numbers. If max_len is 0, window grows indefinitely. A finite value
// saves memory by deleting old information. Old information is only
// deleted if it helps save memory.
template <class BlockData>
class WindowBlocks {
 public:
	WindowBlocks(NumBytes max_len)
	: window(),
		max_len(max_len)
	{}

	void update_block(NumBytes start, NumBytes len, BlockData data);
	void debug_print();

 private:
	struct Block {
		NumBytes start;
		NumBytes length;
		BlockData data;
	};

	std::list<Block> window;
	NumBytes max_len;
};

/*
// Abstract base class
class SendWindow {
	// Fills 'seq_num' with sequence number of first byte to be send and
	// 'block_length' with length of data block to be sent.
	virtual void get_next_block_to_send(NumBytes& seq_num, NumBytes& block_length) const;
	// Fills char buffer pointed to by 'base' with data to be sent
	// next. Assumes sufficient space is available.
	virtual void fill_data(const char* base) const;
	// Tell window manager that data has been sent.
	virtual void mark_data_as_sent(NumBytes seq_num, NumBytes block_length);
	// Tell window manager that data has been acked.
	virtual void mark_data_as_acked(NumBytes seq_num, NumBytes block_length) override;
	
	virtual MagicId get_magic_id() const;
};

class DefaultSendWindow : public SendWindow {
	DefaultSendWindow()
		: buffer(),
			buffer_length(0),
			buffer_start_seq_num(0)
	{}

	// Fills 'seq_num' with sequence number of first byte to be send and
	// 'block_length' with length of data block to be sent.
	virtual void get_next_block_to_send(NumBytes& seq_num, NumBytes& block_length) const override;
	// Fills char buffer pointed to by 'base' with data to be sent
	// next. Assumes sufficient space is available.
	virtual void fill_data(const char* base) const override;
	// Tell window manager that data has been sent.
	virtual void mark_data_as_sent(NumBytes seq_num, NumBytes block_length) override;
	// Tell window manager that data has been acked.
	virtual void mark_data_as_acked(NumBytes seq_num, NumBytes block_length) override;
	
	virtual MagicId get_magic_id() const { return 0x1};


 private:
	// Maximum size of send window (and hence of buffer).
	constexpr max_window_size = snd_window_size;

	// Buffer for data bytes.
	std::queue<char> buffer;
	// Length of buffer in bytes.
  NumBytes buffer_length;
	// Sequence number of first byte in buffer.
	NumBytes buffer_start_seq_num;

	struct DataBlock {
		NumBytes seq_num;
		NumBytes length;
		// Number of times this block has been transmitted
		int num_transmissions;
	};
	// List of starting and ending addresses of blocks of acked bytes
	std::list<DataBlock> acked_blocks;
};
*/
#endif
