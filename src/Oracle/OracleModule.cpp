// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Oracle/OracleModule.h"
#include "Oracle/OracleDBConnection.h"
#include "Generic/common/FeatureModule.h"

extern "C" DLL_PUBLIC void* setup_Oracle() {
	DatabaseConnection::registerImplementation<OracleDBConnection>("oracle");
	return FeatureModule::setup_return_value();
}
