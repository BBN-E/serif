// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Generic/names/discmodel/featuretypes/IdFWordFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevTagFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFReducedTagFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFSemiReducedTagFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevWordFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextWordFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFLCWordFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFIdFWordFeatFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFAllIdFWordFeatFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWC8FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWC12FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWC16FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWC20FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevWC8FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevWC12FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevWC16FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevWC20FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextWC8FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextWC12FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextWC16FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextWC20FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFSingleWordNameListFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFStemVariantsFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFDefiniteWordFeature.h"
#include "Generic/names/discmodel/featuretypes/IdFWordWithoutAl.h"
#include "Generic/names/discmodel/featuretypes/IdFSecondaryDecoderFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevBigramFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextBigramFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevTrigramFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextTrigramFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFCenterTrigramFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevBigramWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextBigramWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevTrigramWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextTrigramWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFCenterTrigramWCFeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFLast3LettersFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFLast2LettersFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFLastLetterFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFFirst3LettersFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFFirst2LettersFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFFirstLetterFeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFRareWordFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWordBigramFreqFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNextBigramCutOffFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrevBigramCutOffFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFWordFreqAndWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFTagBiasFeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFPrev2WC8FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrev2WC12FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrev2WC16FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFPrev2WC20FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNext2WC8FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNext2WC12FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNext2WC16FeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFNext2WC20FeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFCenterTrigramIDFFeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFDomainWCFeatureType.h"
#include "Generic/names/discmodel/featuretypes/IdFSecondaryWCFeatureType.h"

#include "Generic/names/discmodel/featuretypes/IdFInterestingContextsFeatureType.h"

#include "Generic/names/discmodel/PIdFFeatureTypes.h"


bool PIdFFeatureTypes::_instantiated = false;

void PIdFFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType
	_new IdFWordWithoutAl();
	_new IdFDefinteWordFeature();

	_new IdFWordFeatureType();
	_new IdFPrevTagFeatureType();
	_new IdFReducedTagFeatureType();
	_new IdFSemiReducedTagFeatureType();
	_new IdFPrevWordFeatureType();
	_new IdFNextWordFeatureType();
	_new IdFLCWordFeatureType();
	_new IdFIdFWordFeatFeatureType();
	_new IdFAllIdFWordFeatFeatureType();
	_new IdFWC8FeatureType();
	_new IdFWC12FeatureType();
	_new IdFWC16FeatureType();
	_new IdFWC20FeatureType();
	_new IdFPrevWC8FeatureType();
	_new IdFPrevWC12FeatureType();
	_new IdFPrevWC16FeatureType();
	_new IdFPrevWC20FeatureType();
	_new IdFNextWC8FeatureType();
	_new IdFNextWC12FeatureType();
	_new IdFNextWC16FeatureType();
	_new IdFNextWC20FeatureType();
	_new IdFSingleWordNameListFeatureType();
	_new IdFStemVariantsFeatureType();
	_new IdFSecondaryDecoderFeatureType();
	_new IdFPrevBigramFeatureType();
	_new IdFNextBigramFeatureType();
	_new IdFPrevTrigramFeatureType();
	_new IdFNextTrigramFeatureType();
	_new IdFCenterTrigramFeatureType();
	_new IdFPrevBigramWCFeatureType();
	_new IdFNextBigramWCFeatureType();
	_new IdFPrevTrigramWCFeatureType();
	_new IdFNextTrigramWCFeatureType();
	_new IdFCenterTrigramWCFeatureType();

	_new IdFLast3LettersFeatureType();
	_new IdFLast2LettersFeatureType();
	_new IdFLastLetterFeatureType();
	_new IdFFirst3LettersFeatureType();
	_new IdFFirst2LettersFeatureType();
	_new IdFFirstLetterFeatureType();

	_new IdFRareWordFeatureType();
	_new IdFWordBigramFreqFeatureType();
	_new IdFNextBigramCutOffFeatureType();
	_new IdFPrevBigramCutOffFeatureType();
	_new IdFWordFreqAndWCFeatureType();
	_new IdFTagBiasFeatureType();

	_new IdFPrev2WC8FeatureType();
	_new IdFPrev2WC12FeatureType();
	_new IdFPrev2WC16FeatureType();
	_new IdFPrev2WC20FeatureType();
	_new IdFNext2WC8FeatureType();
	_new IdFNext2WC12FeatureType();
	_new IdFNext2WC16FeatureType();
	_new IdFNext2WC20FeatureType();
	
	_new IdFCenterTrigramIDFFeatureType();

	_new IdFDomainWCFeatureType();
	_new IdFSecondaryWCFeatureType();

	_new IdFInterestingContextsFeatureType();
}
