// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/morphSelection/ParseSeeder.h"
#include "Generic/morphSelection/xx_ParseSeeder.h"




boost::shared_ptr<ParseSeeder::Factory> &ParseSeeder::_factory() {
	static boost::shared_ptr<ParseSeeder::Factory> factory(new GenericParseSeederFactory());
	return factory;
}

