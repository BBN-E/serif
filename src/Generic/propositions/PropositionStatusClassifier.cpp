// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/propositions/PropositionStatusClassifier.h"
#include "Generic/propositions/xx_PropositionStatusClassifier.h"




boost::shared_ptr<PropositionStatusClassifier::Factory> &PropositionStatusClassifier::_factory() {
	static boost::shared_ptr<PropositionStatusClassifier::Factory> factory(new DefaultPropositionStatusClassifierFactory());
	return factory;
}

