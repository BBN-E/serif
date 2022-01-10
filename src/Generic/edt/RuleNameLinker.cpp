// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/edt/RuleNameLinker.h"
#include "Generic/edt/xx_RuleNameLinker.h"




boost::shared_ptr<RuleNameLinker::Factory> &RuleNameLinker::_factory() {
	static boost::shared_ptr<RuleNameLinker::Factory> factory(new GenericRuleNameLinkerFactory());
	return factory;
}

