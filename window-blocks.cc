#include "window-blocks.hh"

using namespace std;


template <class BlockData>
void WindowBlocks<BlockData>::update_block(NumBytes start, NumBytes length, BlockData data) {
	if (window.size() == 0) {
		window.push_back(Block {start, length, data});
		return;
	}

	auto block = window.begin();
	for (; block != window.end(); ++ block) {
		if (block == -- window.end() && block->start > start) { 
			// Block is before any existing block
			window.push_back(Block {start, 0, data}); // Add sentinel element
			block = -- window.end();
		}
		if (block->start > start)
			continue;
		// Process start of block to update.
		if (block->start + block->length < start) { // fresh block
			window.insert(block, Block {start, length, data});
			-- block;
		}
		else if (block->start + block->length == start) {
			if (block->data == data) // append to block
				block->length += length;
			else { // fresh block
				window.insert(block, Block {start, length, data});
				-- block;
			}
		}
		else {
			if (block->data != data) { // truncate block and insert new one
				if (start > block->start && 
						start + length >= block->start + block->length) {
					block->length = start - block->start;
					window.insert(block, Block {start, length, data});
					-- block;
				}
				else if (start > block->start) {
					// The new block is in the middle of an older block. So
					// 3-way split.
					window.insert(block, Block {start + length, 
								block->start + block->length - start - length, block->data});
					window.insert(block, Block {start, length, data});
					block->length = start - block->start;
					-- block;
				}
				else if (start == block->start) {
					if (start + length >= block->start + block->length) {
						block->start = start;
						block->length = length;
						block->data = data;
					}
					else {
						// Split.
						window.insert(++ block, Block {start, length, data});
						-- block; -- block;
						block->length = block->start + block->length - start - length;
						block->start = start + length;
						++ block;
					}
					// Merge with previous block if possible
					if (block != window.end() && data == (++ block)->data) {
						block->length += length;
						auto temp_block = -- block;
						++ block;
						window.erase(temp_block);
					}
					else
						-- block;
				}
			}
			else
				block->length = max(start + length - block->start, block->length);
		}
		break;
	}

	// Now `block` points to a block that has newly updated data. We
	// still need to make sure that it does not collide with the data
	// ahead of it. That is `block->start + block->length <
	// (--block)->start`. We will try to merge with the next block if
	// possible.
	bool flag_decrement = true;
	while (block != window.begin()) {
		auto block_ahead = -- block;
		++ block;
		if (block->start + block->length >= block_ahead->start) { // There is a collision
			if (block->data == block_ahead->data) { // Merge the blocks
				block_ahead->length = block_ahead->length + block_ahead->start - block->start;
				block_ahead->start = block->start;
				window.erase(block);
				block = block_ahead;
			}
			else if (block->start + block->length > block_ahead->start) { // Separate 'em
				block_ahead->length = block_ahead->start + block_ahead->length - block->start - block->length;
				block_ahead->start = block->start + block->length;
			}
			if (block_ahead->length <= 0) {
				window.erase(block_ahead);
				//++ block;
				flag_decrement = false;
			}
		}
		else
			break;
		if (block != window.begin() && flag_decrement)
				-- block;
	}

	// Delete any information older than max_len to save memory.
	if (max_len > 0) {
		NumBytes latest_byte = window.front().start + window.front().length;
		for (auto block = -- window.end(); block != window.begin();) {
			if (latest_byte - block->start - block->length > max_len) {
				auto temp_block = block;
				-- block;
				window.erase(temp_block);
			}
			else
				break;
		}
	}
}

template <class BlockData>
const Block& WindowBlocks<BlockData>::get_block(NumBytes byte) {
  for(const auto& x : window)
    if (x.start <= byte && x.start + x.length > byte)
      return x;
  return BlockData {0, 0, Block()}
}

template <class BlockData>
void WindowBlocks<BlockData>::debug_print() {
	for(const auto& x : window)
		std::cout << x.start << ":" << x.length << "(" << int(x.data) << ") ";
	std::cout << std::endl;
}

#include "send-window.hh"

template class WindowBlocks<int>;
template class WindowBlocks<SendWindow::BlockData>;
