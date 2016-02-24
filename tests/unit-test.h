#ifndef TESTS_UNIT_TEST_H
#define TESTS_UNIT_TEST_H

// Abstract class from which classes that perform unit tests derive.
class UnitTest {
 public:
	virtual bool run_tests() const=0;
};

#endif
