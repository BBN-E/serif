#include "MinMaxHeap.h"

class Comp {
public:
	Comp() {}
	int operator()(int a, int b) const {
		if (a<b) {
			return -1;
		} else if (a == b) {
			return 0;
		} else {
			return 1;
		}
	}
};


void MinMaxHeapTest::testMinMaxHeap() {
	srand(4);
	MinMaxHeap<int,Comp> foo;
	std::vector<int> source, simul;

	for (int i=0; i<1000; ++i) {
		source.push_back(i);
	}

	// test runs
	for (int i=0; i<1000; ++i) {
		size_t sz = rand() % 23 + 1;
		std::random_shuffle(source.begin(), source.end());
		foo.clear();
		simul.clear();

		std::cout << "Shuffle is ";
		for (size_t j=0; j<sz; ++j) {
			std::cout << source[j] << " " ;
			foo.insert(source[j]);
			simul.push_back(source[j]);
		}
		sort(simul.begin(), simul.end());
		std::cout << std::endl;

		for (size_t j=0; j<sz/2; ++j) {
			int val = foo.peekMin();
			if (val != simul[j]) {
				return;
			}
			foo.popMin();
		}

		size_t limit = sz/2;
		if (sz % 2 == 1) ++limit;
		for (size_t j=sz-1; j>=limit; --j) {
			int val = foo.peekMax();
			if (val != simul[j]) {
				return;
			}
			foo.popMax();
		}
		std::cout << "Passed test " << i << std::endl;
	}
}
