// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/SignificantConstitOracle.h"
#include "Generic/parse/xx_SignificantConstitOracle.h"




boost::shared_ptr<SignificantConstitOracle::Factory> &SignificantConstitOracle::_factory() {
	static boost::shared_ptr<SignificantConstitOracle::Factory> factory(new GenericSignificantConstitOracleFactory());
	return factory;
}

