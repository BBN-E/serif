// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/HeapChecker.h"

#ifdef ENABLE_LEAK_DETECTION
	// apparently leak detection screws up _heapchk(), so if that's
	// turned on, then just define a dummy checkHeap()
	void HeapChecker::checkHeap(const char *location) {
	}
#else

#include <iostream>

using namespace std;


void HeapChecker::checkHeap(const char *location) {
#ifdef _WIN32
	int result = _heapchk();
	if (result == _HEAPOK)
		return;
	cerr << "Oh no! Corrupt heap: ";
	switch (result) {
		case _HEAPBADBEGIN: cerr << "_HEAPBADBEGIN"; break;
		case _HEAPBADNODE: cerr << "_HEAPBADNODE"; break;
		case _HEAPBADPTR: cerr << "_HEAPBADPTR"; break;
		case _HEAPEMPTY: cerr << "_HEAPEMPTY"; break;
	}
	cerr << " at: " << location << "\n";
#else 
	cerr << "HeapChecker::checkHeap() is not available on this platform!" << endl;
#endif // #ifdef _WIN32
}

#endif // #ifdef ENABLE_LEAK_DETECTION
