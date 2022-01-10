// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/relations/RelationUtilities.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/common/UTF8OutputStream.h"

UTF8OutputStream RelationUtilities::_debugStream;

boost::shared_ptr<RelationUtilities::Factory> &RelationUtilities::_factory() {
	static boost::shared_ptr<RelationUtilities::Factory> factory(new DefaultRelationUtilitiesFactory());
	return factory;
}
