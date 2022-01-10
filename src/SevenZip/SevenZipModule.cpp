// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#error "Do not compile this unless you know what you are doing.  We may not be allowed to use LGPL software."

#include "Generic/common/leak_detection.h"

#include "SevenZip/SevenZipModule.h"
#include "SevenZip/SevenZipDecompressor.h"
#include "Generic/common/Decompressor.h"
#include "Generic/common/FeatureModule.h"

extern "C" DLL_PUBLIC void* setup_SevenZip() {
	Decompressor::registerImplementation(".7z", _new SevenZipDecompressor());
	return FeatureModule::setup_return_value();
}
