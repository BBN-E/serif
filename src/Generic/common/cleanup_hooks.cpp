// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/cleanup_hooks.h"

#include <vector>
#include <boost/foreach.hpp>

namespace {
	std::vector<CleanupFunction>& _cleanupFunctions() {
		static std::vector<CleanupFunction> cleanupFunctions;
		return cleanupFunctions;
	}
}

void addCleanupHook(CleanupFunction hook) {
	_cleanupFunctions().push_back(hook);
}

/** Run all registered cleanup hooks. */
void runCleanupHooks() {
	BOOST_FOREACH(CleanupFunction f, _cleanupFunctions()) {
		f();
	}
}

