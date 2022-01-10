// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Generic/docRelationsEvents/featuretypes/TAConnectingStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TADistanceFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAPropFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAPropParticipantFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TASentenceLocationFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAVerbalRelationAndDistanceFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TATimexStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAMentionConnectingStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAGoverningPrepFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAAbbrevConnectingStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAPOSConnectingStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAStemmedConnectingStringFeatureType.h"
#include "Generic/docRelationsEvents/featuretypes/TAParsePathBetweenFeatureType.h"


#include "Chinese/docRelationsEvents/ch_RelationTimexArgFeatureTypes.h"

bool ChineseRelationTimexArgFeatureTypes::_instantiated = false;

void ChineseRelationTimexArgFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new TAConnectingStringFeatureType();
	_new TADistanceFeatureType();
	_new TAPropFeatureType();
	_new TAPropParticipantFeatureType();
	_new TASentenceLocationFeatureType();
	_new TAVerbalRelationAndDistanceFeatureType();
	_new TATimexStringFeatureType();
	_new TAMentionConnectingStringFeatureType();
	_new TAGoverningPrepFeatureType();
	_new TAAbbrevConnectingStringFeatureType();
	_new TAPOSConnectingStringFeatureType();
	_new TAStemmedConnectingStringFeatureType();
	_new TAParsePathBetweenFeatureType();
	
}
