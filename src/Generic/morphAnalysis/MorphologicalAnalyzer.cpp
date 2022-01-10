// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/morphAnalysis/MorphologicalAnalyzer.h"
#include "Generic/morphAnalysis/xx_MorphologicalAnalyzer.h"


boost::shared_ptr<MorphologicalAnalyzer::Factory> &MorphologicalAnalyzer::_factory() {
	static boost::shared_ptr<MorphologicalAnalyzer::Factory> factory(new GenericMorphologicalAnalyzerFactory());
	return factory;
}

