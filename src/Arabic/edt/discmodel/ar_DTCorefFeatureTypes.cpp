// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Arabic/edt/discmodel/ar_DTCorefFeatureTypes.h"

#include "Arabic/edt/discmodel/featuretypes/AdjNationality.h"

void ArabicDTCorefFeatureTypes::ensureFeatureTypesInstantiated() {

	if (DTCorefFeatureTypes::_instantiated)
		return;

	// get the language independent featureTypes
	DTCorefFeatureTypes::ensureBaseFeatureTypesInstantiated();


	// here add the additional arabic only features
	_new ArabicAdjNationality();
}
