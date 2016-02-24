#ifndef TEST_UNIT_TEST_WINDOW_H
#define TEST_UNIT_TEST_WINDOW_H

#include "unit-test.h"
#include "../send-window.hh"

class UnitTestWindow : public UnitTest {
 public:
  virtual bool run_tests() const override {
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
		return true;
	}
};

#endif
