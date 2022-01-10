// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.
#include "Chinese/edt/discmodel/ch_DTCorefFeatureTypes.h"


void ChineseDTCorefFeatureTypes::ensureFeatureTypesInstantiated() {

	if (DTCorefFeatureTypes::_instantiated)
	return;

	// get the language independent featureTypes
	DTCorefFeatureTypes::ensureBaseFeatureTypesInstantiated();

	// here add the additional chinese only features

}
