// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/events/stat/en_EventTriggerFeatureTypes.h"

#include "Generic/events/stat/featuretypes/ETWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETSubjectFeatureType.h"
#include "Generic/events/stat/featuretypes/ETObjectFeatureType.h"
#include "Generic/events/stat/featuretypes/ETTriggerArgsFeatureType.h"
#include "Generic/events/stat/featuretypes/ETSubjectWCFeatureType.h"
#include "Generic/events/stat/featuretypes/ETObjectWCFeatureType.h"
#include "Generic/events/stat/featuretypes/ETTriggerArgsWCFeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWordBigramFeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWordBigramFeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWC8FeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWC12FeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWC16FeatureType.h"
#include "Generic/events/stat/featuretypes/ETNextWC20FeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWC8FeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWC12FeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWC16FeatureType.h"
#include "Generic/events/stat/featuretypes/ETPrevWC20FeatureType.h"
#include "Generic/events/stat/featuretypes/ETWC8FeatureType.h"
#include "Generic/events/stat/featuretypes/ETWC12FeatureType.h"
#include "Generic/events/stat/featuretypes/ETWC16FeatureType.h"
#include "Generic/events/stat/featuretypes/ETWC20FeatureType.h"
#include "Generic/events/stat/featuretypes/ETLowercaseWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETStemmedWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETDocTopicFeatureType.h"
#include "Generic/events/stat/featuretypes/ETDocTopicWCFeatureType.h"
#include "Generic/events/stat/featuretypes/ETPOSWordFeatureType.h"
#include "Generic/events/stat/featuretypes/ETNominalPremodFeatureType.h"
#include "Generic/events/stat/featuretypes/ETCopulaFeatureType.h"
#include "English/events/stat/featuretypes/en_ETWordNetFeatureType.h"
#include "English/events/stat/featuretypes/en_ETSubjectWNFeatureType.h"
#include "English/events/stat/featuretypes/en_ETObjectWNFeatureType.h"
#include "English/events/stat/featuretypes/en_ETTriggerArgsWNFeatureType.h"

bool EnglishEventTriggerFeatureTypes::_instantiated = false;

void EnglishEventTriggerFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new ETWordFeatureType();
	_new ETNextWordFeatureType();
	_new ETPrevWordFeatureType();
	_new ETNextWordBigramFeatureType();
	_new ETPrevWordBigramFeatureType();
	_new ETLowercaseWordFeatureType();
	_new ETStemmedWordFeatureType();

	_new ETSubjectFeatureType();
	_new ETObjectFeatureType();
	_new ETTriggerArgsFeatureType();
	_new ETSubjectWCFeatureType();
	_new ETObjectWCFeatureType();
	_new ETTriggerArgsWCFeatureType();
	
	_new EnglishETSubjectWNFeatureType();
	_new EnglishETObjectWNFeatureType();
	_new EnglishETTriggerArgsWNFeatureType();

	_new ETPrevWC8FeatureType();
	_new ETPrevWC12FeatureType();
	_new ETPrevWC16FeatureType();
	_new ETPrevWC20FeatureType();

	_new ETNextWC8FeatureType();
	_new ETNextWC12FeatureType();
	_new ETNextWC16FeatureType();
	_new ETNextWC20FeatureType();

	_new ETWC8FeatureType();
	_new ETWC12FeatureType();
	_new ETWC16FeatureType();
	_new ETWC20FeatureType();

	_new EnglishETWordNetFeatureType();

	_new ETDocTopicFeatureType();
	_new ETDocTopicWCFeatureType();

	_new ETPOSWordFeatureType();
	_new ETNominalPremodFeatureType();
	_new ETCopulaFeatureType();
}
