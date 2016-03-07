#include "unit-test.h"
#include "unit-test-window.h"

#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

int main() {
	vector< pair<UnitTest*, string> > unit_tests;
	unit_tests.push_back(make_pair(new UnitTestWindow(), string("Window")));

	std::streambuf *coutbuf = cout.rdbuf(); //save old buf
	for (const auto& test : unit_tests) {
		ofstream out(string("tests/output/") + test.second + string(".output"));
		cout.rdbuf(out.rdbuf());
		bool res = test.first->run_tests();
		cout.rdbuf(coutbuf);
		out.close();

		// Check if output was as expected
		ifstream actual(string("tests/output/") + test.second + string(".output"));
		ifstream exp(string("tests/expected_output/") + test.second + string(".output"));
		while (!actual.eof() && !exp.eof()) {
			char a, e;
			actual >> a; exp >> e;
			if (a != e) {
				res = false;
				break;
			}
		}
		if (!actual.eof() || !exp.eof())
			res = false;

		if (res)
			cout << "Unit Tests Passed: '" << test.second << "'." << endl;
		else
			cout << "Unit Tests Failed: '" << test.second << "'." << endl;
	}
}
