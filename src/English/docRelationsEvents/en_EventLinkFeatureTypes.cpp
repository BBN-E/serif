// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/docRelationsEvents/en_EventLinkFeatureTypes.h"

#include "Generic/docRelationsEvents/featuretypes/ELSameArgFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELArgConflictFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELDistanceFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELPriorFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameCoreNPFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerDistFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerSameArgFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameArgHWFeatureType.h"

#include "Generic/docRelationsEvents/featuretypes/ELSameArgRedFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELArgConflictRedFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELDistanceRedFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameCoreNPRedFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerRedFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerDistRedFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELSameTriggerSameArgRedFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/ELSameArgHWRedFeatureType.h"

#include "Generic/docRelationsEvents/featuretypes/ELTriggerWordsFeatureType.h"
#include "English/docRelationsEvents/featuretypes/en_ELStemmedTriggerWordsFT.h"

bool EnglishEventLinkFeatureTypes::_instantiated = false;

void EnglishEventLinkFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new ELSameArgFeatureType();
	_new EnglishELSameTriggerFeatureType();
	_new EnglishELSameTriggerDistFeatureType();
	_new ELDistanceFeatureType();
	_new ELPriorFeatureType();
	_new ELArgConflictFeatureType();
	_new ELSameCoreNPFeatureType();
	_new ELSameArgHWFeatureType();
	_new EnglishELSameTriggerSameArgFeatureType();

	
	_new ELSameArgRedFeatureType();
	_new EnglishELSameTriggerRedFeatureType();
	_new EnglishELSameTriggerDistRedFeatureType();
	_new ELDistanceRedFeatureType();
	_new ELArgConflictRedFeatureType();
	_new ELSameCoreNPRedFeatureType();
	_new ELSameArgHWRedFeatureType();
	_new EnglishELSameTriggerSameArgRedFeatureType();

	_new ELTriggerWordsFeatureType();
	_new EnglishELStemmedTriggerWordsFeatureType();
}
