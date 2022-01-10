// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.
#include "Spanish/edt/discmodel/es_DTCorefFeatureTypes.h"


void SpanishDTCorefFeatureTypes::ensureFeatureTypesInstantiated() {

	if (DTCorefFeatureTypes::_instantiated)
	return;

	// get the language independent featureTypes
	DTCorefFeatureTypes::ensureBaseFeatureTypesInstantiated();

	// here add the additional spanish only features

}
