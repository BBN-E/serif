// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/EvalSpecificRules.h"
#include "Generic/common/xx_EvalSpecificRules.h"


boost::shared_ptr<EvalSpecificRules::Factory> &EvalSpecificRules::_factory() {
	static boost::shared_ptr<EvalSpecificRules::Factory> factory(new GenericEvalSpecificRulesFactory());
	return factory;
}

