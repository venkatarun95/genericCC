#ifndef WINDOW_BLOCKS_HH
#define WINDOW_BLOCKS_HH

#include "configs.hh"
#include "tcp-header.hh"

#include <iostream>
#include <limits>
#include <list>
#include <queue>

// Support class to maintain information about bytes in a window.
//
// It is optimized under the assumption that adjacent bytes are highly
// likely to have the same data.
//
// Note that wrapping around of sequence numbers is not yet
// supported. It is assumed that newer bytes have higher sequence
// numbers. If max_len is 0, window grows indefinitely. A finite value
// saves memory by deleting old information. Old information is only
// deleted if it helps save memory.
template <class BlockData>
class WindowBlocks {
 public:  
	struct Block {
		NumBytes start;
		NumBytes length;
		BlockData data;
	};

	WindowBlocks(NumBytes max_len)
	: window(),
		max_len(max_len)
	{}

	void update_block(NumBytes start, NumBytes len, BlockData data);
  const std::list<Block>& get_block_list() const {return window;};
  // Get data stored for a particulat byte. If no data is stored,
  // length is 0 in the returned block. This scans linearly from the
  // highest byte index.
  Block get_block(NumBytes byte) const;

	void debug_print();

 private:
	std::list<Block> window;
	NumBytes max_len;
};

#endif // WINDOW_BLOCKS_HH
