// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/SemTreeBuilder.h"
#include "Generic/propositions/xx_SemTreeBuilder.h"




boost::shared_ptr<SemTreeBuilder::Factory> &SemTreeBuilder::_factory() {
	static boost::shared_ptr<SemTreeBuilder::Factory> factory(new DefaultSemTreeBuilderFactory());
	return factory;
}

