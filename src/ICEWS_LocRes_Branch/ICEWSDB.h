// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWSDB_H
#define ICEWSDB_H

#include "Generic/database/DatabaseConnection.h"

namespace ICEWS {
	DatabaseConnection_ptr getSingletonIcewsDb();
	DatabaseConnection_ptr getSingletonIcewsStoriesDb();
	DatabaseConnection_ptr getSingletonIcewsStorySerifXMLDb();
	DatabaseConnection_ptr getSingletonIcewsOutputDb();
	DatabaseConnection_ptr getSingletonIcewsGeonamesDb();
}

#endif
