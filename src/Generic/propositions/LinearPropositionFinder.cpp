// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/LinearPropositionFinder.h"
#include "Generic/propositions/xx_LinearPropositionFinder.h"




boost::shared_ptr<LinearPropositionFinder::Factory> &LinearPropositionFinder::_factory() {
	static boost::shared_ptr<LinearPropositionFinder::Factory> factory(new DefaultLinearPropositionFinderFactory());
	return factory;
}

