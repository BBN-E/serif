// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/events/stat/en_EventAAFeatureTypes.h"

#include "Generic/events/stat/featuretypes/AAPropFeatureType.h"
#include "Generic/events/stat/featuretypes/AADistanceFeatureType.h"
#include "Generic/events/stat/featuretypes/AADistanceRTFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandETFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPropParticipantFeatureType.h"
#include "Generic/events/stat/featuretypes/AAConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPOSConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AAStemmedConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AAAbbrevConnectingStringFeatureType.h"
#include "Generic/events/stat/featuretypes/AAPropWCFeatureType.h"
#include "Generic/events/stat/featuretypes/AAFirstPassArgFeatureType.h"
#include "Generic/events/stat/featuretypes/AANCandOfSameTypeFeatureType.h"
#include "Generic/events/stat/featuretypes/AAParsePathBetweenFeatureType.h"
#include "Generic/events/stat/featuretypes/AAParsePathBetweenETFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandHeadwordFeatureType.h"
#include "Generic/events/stat/featuretypes/AACandTriggerHWsFeatureType.h"

// distributional knowledge
#include "Generic/events/stat/featuretypes/AADKAnchorAllCidV1FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAnchorAllCidV6FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAnchorArgPmiAndPropV20RTFeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAnchorArgPmiAndPropV6FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAnchorArgSimAndPropV19FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAssocPropPmiAndPropV1FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAssocPropSimAndPropV21FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKAssocPropSimAndPropV7FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKCausalScoreAndPropV8RTFeatureType.h"
#include "Generic/events/stat/featuretypes/AADKContentWordsAPPmiV3FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKContentWordsV3FeatureType.h"
#include "Generic/events/stat/featuretypes/AADKMaxSubObjScoreAndPropV14FeatureType.h"



bool EnglishEventAAFeatureTypes::_instantiated = false;

void EnglishEventAAFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new AAPropFeatureType();
	_new AAPropWCFeatureType();
	_new AADistanceFeatureType();
	_new AADistanceRTFeatureType();
	_new AACandETFeatureType();
	_new AACandHeadwordFeatureType();
	_new AACandTriggerHWsFeatureType();
	_new AAPropParticipantFeatureType();
	_new AAConnectingStringFeatureType();
	_new AAPOSConnectingStringFeatureType();
	_new AAStemmedConnectingStringFeatureType();
	_new AAAbbrevConnectingStringFeatureType();
	_new AAFirstPassArgFeatureType();
	_new AANCandOfSameTypeFeatureType();
	_new AAParsePathBetweenFeatureType();
	_new AAParsePathBetweenETFeatureType();

	// distributional knowledge
	_new AADKAnchorAllCidV1FeatureType();
        _new AADKAnchorAllCidV6FeatureType();
	_new AADKAnchorArgPmiAndPropV20RTFeatureType();
	_new AADKAnchorArgPmiAndPropV6FeatureType();
	_new AADKAnchorArgSimAndPropV19FeatureType();
	_new AADKAssocPropPmiAndPropV1FeatureType();
	_new AADKAssocPropSimAndPropV21FeatureType();
	_new AADKAssocPropSimAndPropV7FeatureType();
	_new AADKCausalScoreAndPropV8RTFeatureType();
	_new AADKContentWordsAPPmiV3FeatureType();
	_new AADKContentWordsV3FeatureType();
	_new AADKMaxSubObjScoreAndPropV14FeatureType();
}
