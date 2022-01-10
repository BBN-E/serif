// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifdef LEAK_DETECTION_H
	#error "This file is being used incorrectly. It should appear once at the top of each .c and .cpp file."
#endif
#define LEAK_DETECTION_H


// With leak detection enabled, a message is printed at exit-time listing
// all objects left on the heap.
#ifdef _DEBUG
// As of 2004-02-12, this is off by default -- SRS
// (uncomment the following line to re-enable it.)
//#define ENABLE_LEAK_DETECTION
#endif


#ifdef ENABLE_LEAK_DETECTION

	#pragma warning(disable: 4291)
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
#if defined(_WIN32)
	#include <crtdbg.h>
#endif
	#define _new new(1, __FILE__, __LINE__)

#else

	#define _new new

#endif
