#include "unit-test.h"
#include "unit-test-window.h"

#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

int main() {
	vector< pair<UnitTest*, string> > unit_tests;
	unit_tests.push_back(make_pair(new UnitTestWindow(), string("Window")));

	for (const auto& test : unit_tests) {
		bool res = test.first->run_tests();
		if (res)
			cout << "Unit Test Passed: '" << test.second << "'." << endl;
		else
			cout << "Unit Test Failed: '" << test.second << "'." << endl;
	}
}
