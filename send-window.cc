#include "send-window.hh"

using namespace std;


template <class BlockData>
void WindowBlocks<BlockData>::update_block(NumBytes start, NumBytes length, BlockData data) {
	if (window.size() == 0) {
		window.push_back(Block {start, length, data});
		return;
	}

	auto block = window.begin();
	for (; block != window.end(); ++ block) {
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
				block->length = start - block->start;
				window.insert(block, Block {start, length, data});
				-- block;
			}
			else
				block->length = start + length - block->start;
		}
		break;
	}

	// Now `block` points to a block that has newly updated data. We
	// still need to make sure that it does not collide with the data
	// ahead of it. That is `block->start + block->length <
	// (--block)->start`. We will try to merge with the next block if
	// possible.
	if (block != window.begin()) {
		auto block_ahead = -- block;
		++ block;
		if (block->start + block->length >= block_ahead->start) { // There is a collision
			if (block->data == block_ahead->data) { // Merge the blocks
				block_ahead->length = block_ahead->length + block_ahead->start - block->start;
				block_ahead->start = block->start;
				window.erase(block);
			}
			else if (block->start + block->length > block_ahead->start) { // Separate 'em
				block_ahead->length -= block->start + block->length - block_ahead->start;
				block_ahead->start = block->start + block->length;
			}
			if (block_ahead->length <= 0)
				window.erase(block_ahead);
		}
	}

	// Delete any information older than max_len to save memory.
	if (max_len > 0) {
		NumBytes latest_byte = window.front().start + window.front().length;
		for (auto block = window.end(); block != window.begin();) {
			if (block->start + block->length - latest_byte > max_len) {
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
void WindowBlocks<BlockData>::debug_print() {
	for(const auto& x : window)
		std::cout << x.start << ":" << x.length << "(" << x.data << ") ";
	std::cout << std::endl;
}

template class WindowBlocks<int>;

/*
void SendWindow::get_next_block_to_send(NumBytes& seq_num, 
																				NumBytes& block_length) const {
	
}

void SendWindow::mark_data_as_sent(NumBytes seq_num, NumBytes block_length) {
	NumBytes next_limit = numeric_limits<NumBytes>::max();
	for (auto& block = acked_blocks.begin(); block != acked_blocks.crend(); ++block) {

		
		
		// If block fits neatly at the end of a previous block
		if (block->seq_num + block->length == seq_num) {
			if (block->num_transmissions == 1)
				block->length += block_length;
			else
				acked_blocks.emplace_front({seq_num, block_length, 1});
			
			// If it collides with the next block.
			if (seq_num + block_length >= next_limit) {
				// Split.
				acked_blocks.acked_blocks(block, {next_seq_num, seq_num + block_length - next_limit});
				-- block;
				block
			}
			break;
		}
	}
}*/


