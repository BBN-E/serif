// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAP_CHECKER_H
#define HEAP_CHECKER_H

// This is for testing for memory corruption. 
// HeapChecker::checkHeap() calls _heapchk() and if there is an error
// it prints out a message indicating which error was detected. What
// actually seems to happen, though, when the heap is corrupt, is
// that _heapchk() crashes, which is still much better than nothing,
// because the corruption can be "detected" arbitrarily soon after
// it occurs.

class HeapChecker {
public:
	static void checkHeap(const char *location);
};

#endif
