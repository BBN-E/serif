// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "MySQL/MySQLModule.h"
#include "MySQL/MySQLDBConnection.h"
#include "Generic/common/FeatureModule.h"

extern "C" DLL_PUBLIC void* setup_MySQL() {
	DatabaseConnection::registerImplementation<MySQLDBConnection>("mysql");
	return FeatureModule::setup_return_value();
}
