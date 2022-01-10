// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Chinese/events/stat/ch_EventAAFeatureTypes.h"

#include "Generic/events/stat/featuretypes/AAAbbrevConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandETFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandHeadwordFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandTriggerHWsFeatureType.h"
#include "Generic/events/stat/featuretypes/AAConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AADistanceFeatureType.h"
#include "Generic/events/stat/featuretypes/AADistanceRTFeatureType.h"
#include "Generic/events/stat/featuretypes/AAFirstPassArgFeatureType.h"
#include "Generic/events/stat/featuretypes/AANCandOfSameTypeFeatureType.h"
#include "Generic/events/stat/featuretypes/AAParsePathBetweenETFeatureType.h"
#include "Generic/events/stat/featuretypes/AAParsePathBetweenFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPOSConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPropFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPropParticipantFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPropWCFeatureType.h"

#include "Chinese/events/stat/featuretypes/ch_AACandLastCharFeatureType.h"



bool ChineseEventAAFeatureTypes::_instantiated = false;

void ChineseEventAAFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new AAAbbrevConnectingStringFeatureType();
	_new AACandETFeatureType();
	_new AACandHeadwordFeatureType();
	_new AACandTriggerHWsFeatureType();
	_new AAConnectingStringFeatureType();
	_new AADistanceFeatureType();
	_new AADistanceRTFeatureType();
	_new AAFirstPassArgFeatureType();
	_new AANCandOfSameTypeFeatureType();
	_new AAParsePathBetweenETFeatureType();
	_new AAParsePathBetweenFeatureType();
	_new AAPOSConnectingStringFeatureType();
	_new AAPropFeatureType();
	_new AAPropParticipantFeatureType();
	_new AAPropWCFeatureType();
	
	_new ChineseAACandLastCharFeatureType();
	
}
