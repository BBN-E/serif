// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Chinese/docRelationsEvents/ch_EventLinkFeatureTypes.h"

#include "Generic/docRelationsEvents/featuretypes/ELSameArgFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELArgConflictFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELDistanceFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELPriorFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameTriggerFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameTriggerSameArgFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameCoreNPFeatureType.h"

bool ChineseEventLinkFeatureTypes::_instantiated = false;

void ChineseEventLinkFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new ELSameArgFeatureType();
	_new ELDistanceFeatureType();
	_new ELPriorFeatureType();
	_new ELArgConflictFeatureType();
	_new ELSameTriggerFeatureType();
	_new ELSameTriggerSameArgFeatureType();
	_new ELSameCoreNPFeatureType();
}
