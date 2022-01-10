// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/events/stat/en_EventModalityFeatureTypes.h"


// to revise later
#include "events/stat/featuretypes/EMWordFeatureType.h"
#include "events/stat/featuretypes/EMNextWordFeatureType.h"
#include "events/stat/featuretypes/EMPrevWordFeatureType.h"
#include "events/stat/featuretypes/EMNextWordBigramFeatureType.h"
#include "events/stat/featuretypes/EMPrevWordBigramFeatureType.h"
#include "events/stat/featuretypes/EMWC8FeatureType.h"
#include "events/stat/featuretypes/EMWC12FeatureType.h"
#include "events/stat/featuretypes/EMWC16FeatureType.h"
#include "events/stat/featuretypes/EMWC20FeatureType.h"
#include "events/stat/featuretypes/EMLowercaseWordFeatureType.h"
#include "events/stat/featuretypes/EMStemmedWordFeatureType.h"
#include "events/stat/featuretypes/EMCopulaFeatureType.h"

#include "events/stat/featuretypes/EMIsLedbyModalWord.h"
#include "events/stat/featuretypes/EMIsFollowedbyIFWord.h"
#include "events/stat/featuretypes/EMIsLedbyIFWord.h"
#include "events/stat/featuretypes/EMIsLedbyAllegedAdverb.h"
#include "events/stat/featuretypes/EMHasNonassertedParent.h"
#include "Generic/events/stat/featuretypes/EMIsNonassertedByRules.h"
#include "Generic/events/stat/featuretypes/EMIsModifierOfNoun.h"
#include "Generic/events/stat/featuretypes/EMIsNounModifierOfNoun.h"

#include "Generic/events/stat/featuretypes/EMNonAssertedIndicatorNoun.h"
#include "Generic/events/stat/featuretypes/EMNonAssertedIndicatorAboveVP.h"
#include "Generic/events/stat/featuretypes/EMNonAssertedIndicatorMDAboveVP.h"
#include "Generic/events/stat/featuretypes/EMNonAssertedIndicatorAboveS.h"
#include "Generic/events/stat/featuretypes/EMNonAssertedIndicatorNearby.h"

#include "Generic/events/stat/featuretypes/EMHasNonAssertedIndicatorNoun.h"
#include "Generic/events/stat/featuretypes/EMHasNonAssertedIndicatorS.h"
#include "Generic/events/stat/featuretypes/EMHasNonAssertedIndicatorVP.h"
#include "Generic/events/stat/featuretypes/EMHasNonAssertedIndicatorMD.h"
#include "Generic/events/stat/featuretypes/EMHasNonAssertedIndicatorNearby.h"

#include "Generic/events/stat/featuretypes/EMisPremodOfNP.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfMention.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfMentionwithMentType.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfMentionwithEntType.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfMentionwithTypes.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfNameMention.h"
#include "Generic/events/stat/featuretypes/EMisPremodOfORGMention.h"

#include "events/stat/featuretypes/EMPOSFeatureType.h"
#include "events/stat/featuretypes/EMPrevPOSFeatureType.h"
#include "events/stat/featuretypes/EMNextPOSFeatureType.h"
#include "events/stat/featuretypes/EMPrevPOSBigramFeatureType.h"
#include "events/stat/featuretypes/EMNextPOSBigramFeatureType.h"


/* might be useful in the future
#include "events/stat/featuretypes/ETDocTopicFeatureType.h"
#include "events/stat/featuretypes/ETDocTopicWCFeatureType.h"
#include "events/stat/featuretypes/ETPOSWordFeatureType.h"
#include "events/stat/featuretypes/ETNominalPremodFeatureType.h"

#include "English/events/stat/featuretypes/en_ETWordNetFeatureType.h"
#include "English/events/stat/featuretypes/en_ETSubjectWNFeatureType.h"
#include "English/events/stat/featuretypes/en_ETObjectWNFeatureType.h"
#include "English/events/stat/featuretypes/en_ETTriggerArgsWNFeatureType.h"
*/


bool EnglishEventModalityFeatureTypes::_instantiated = false;

void EnglishEventModalityFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new EMWordFeatureType();
	_new EMNextWordFeatureType();
	_new EMPrevWordFeatureType();
	_new EMNextWordBigramFeatureType();
	_new EMPrevWordBigramFeatureType();
	_new EMLowercaseWordFeatureType();
	_new EMStemmedWordFeatureType();

	_new EMWC8FeatureType();
	_new EMWC12FeatureType();
	_new EMWC16FeatureType();
	_new EMWC20FeatureType();
	_new EMCopulaFeatureType();

	_new isLedByModalWord();
	_new isLedByIFWord();
	_new isFollowedByIFWord();
	_new isLedByAllegedAdverb();
	_new hasNonAssertedParent();
	_new isNonAssertedByRules();
	_new isModifierOfNoun();
	_new isNounModifierOfNoun();

	_new nonAssertedIndicatorNoun();
	_new nonAssertedIndicatorAboveS();
	_new nonAssertedIndicatorAboveVP();
	_new nonAssertedIndicatorMDAboveVP();
	_new nonAssertedIndicatorNearby();
	_new hasNonAssertedIndicatorNoun();
	_new hasNonAssertedIndicatorS();
	_new hasNonAssertedIndicatorVP();
	_new hasNonAssertedIndicatorMD();
	_new hasNonAssertedIndicatorNearby();

	_new isPremodOfNP();
	_new isPremodOfMention();
	_new isPremodOfMentionWithMentType();
	_new isPremodOfMentionWithEntType();
	_new isPremodOfMentionWithTypes();
	_new isPremodOfNameMention();
	_new isPremodOfORGMention();

	_new EMPOSFeatureType();
	_new EMPrevPOSFeatureType();
	_new EMPrevPOSBigramFeatureType();
	_new EMNextPOSFeatureType();
	_new EMNextPOSBigramFeatureType();

}
