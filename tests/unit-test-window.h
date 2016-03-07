#ifndef TEST_UNIT_TEST_WINDOW_H
#define TEST_UNIT_TEST_WINDOW_H

#include "unit-test.h"
#include "../window-blocks.hh"

#include <iostream>

class UnitTestWindow : public UnitTest {
 public:
  virtual bool run_tests() const override {
		std::cout << "Corner Cases\n";
		WindowBlocks<int> win(0);
		win.update_block(0, 1, 1);
		win.debug_print();
		win.update_block(1, 2, 1); // Whether a new block joins
		win.debug_print();
		win.update_block(3, 1, 2); // This should not join
		win.debug_print();
		win.update_block(10, 2, 2); // Discrete block
		win.debug_print();
		win.update_block(9, 1, 2); // Should join block ahead
		win.debug_print();
		win.update_block(7, 2, 1); // Shouldn't join block ahead
		win.debug_print();
		win.update_block(4, 4, 2); // Join with previous, overlap with next.
		win.debug_print();
		win.update_block(8, 3, 1); // Extend second last block into last.
		win.debug_print();
		win.update_block(10, 2, 1); // Remove last block
		win.debug_print();
		win.update_block(8, 3, 2); // Merge last two blocks except 1 byte
		win.debug_print();
		win.update_block(0, 20, 3); // Merge everything into 1
		win.debug_print();
		win.update_block(5, 5, 2); // Split again
		win.debug_print();
		win.update_block(0, 2, 3); // Do nothing
		win.debug_print();
		win.update_block(0, 2, 2); // Split last block
		win.debug_print();
		win.update_block(0, 2, 4); // Replace block
		win.debug_print();
		win.update_block(0, 2, 3); // Replace and merge blocks
		win.debug_print();
		win.update_block(15, 10, 2); // Modify last bytes; truncate last block
		win.debug_print();
		std::cout << "\nNormal Usage\n";
		WindowBlocks<int> nrm(1000);
		nrm.debug_print();
		nrm.update_block(0, 100, 1);
		nrm.debug_print();
		nrm.update_block(100, 100, 1);
		nrm.debug_print();
		nrm.update_block(200, 100, 1);
		nrm.debug_print();
		nrm.update_block(400, 100, 1); // Packet reorder
		nrm.debug_print();
		nrm.update_block(500, 100, 1);
		nrm.debug_print();
		nrm.update_block(300, 100, 1); // Packet back
		nrm.debug_print();
		nrm.update_block(800, 100, 1); // 2 packet reordering
		nrm.debug_print();
		nrm.update_block(700, 100, 1);
		nrm.debug_print();
		nrm.update_block(600, 100, 1); // Both packets are back
		nrm.debug_print();
		nrm.update_block(200, 100, 1); // No change
		nrm.debug_print();
		nrm.update_block(200, 100, 2); // 3-way split
		nrm.debug_print();
		nrm.update_block(900, 400, 1); // Extend beyond 1000; delete last block
		nrm.debug_print();
		nrm.update_block(100, 300, 1); // Merge from behind oldest byte
		nrm.debug_print();
		return true;
	}
};

#endif
