#ifndef _DUMP_VECTOR_H_
#define _DUMP_VECTOR_H_

#include <iostream>
#include <vector>

void dumpVector(const std::vector<double>& vec) {
	std::vector<double>::const_iterator it = vec.begin();
	for (; it!=vec.end(); ++it) {
		std::wcout << *it << L"\t";
	}
}

#endif

