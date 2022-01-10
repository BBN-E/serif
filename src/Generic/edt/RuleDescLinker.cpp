// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/RuleDescLinker.h"
#include "Generic/edt/xx_RuleDescLinker.h"




boost::shared_ptr<RuleDescLinker::Factory> &RuleDescLinker::_factory() {
	static boost::shared_ptr<RuleDescLinker::Factory> factory(new GenericRuleDescLinkerFactory());
	return factory;
}

