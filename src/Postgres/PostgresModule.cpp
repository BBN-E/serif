// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Postgres/PostgresModule.h"
#include "Postgres/PostgresDBConnection.h"
#include "Generic/common/FeatureModule.h"

extern "C" DLL_PUBLIC void* setup_Postgres() {
	DatabaseConnection::registerImplementation<PostgresDBConnection>("postgresql");
	return FeatureModule::setup_return_value();
}
