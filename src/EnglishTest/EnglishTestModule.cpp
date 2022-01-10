// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "EnglishTest/test/en_UnitTester.h"

#include "Generic/common/cleanup_hooks.h"
#include "Generic/common/FeatureModule.h"
#include "EnglishTestModule.h"

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_EnglishTest() {

	UnitTester::setFactory(boost::shared_ptr<UnitTester::Factory>
		(new EnglishUnitTesterFactory()));

	return FeatureModule::setup_return_value();
}
