// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Arabic/descriptors/discmodel/ar_P1DescFeatureTypes.h"

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordFT.h"
#include "Generic/descriptors/discmodel/featuretypes/FuncParentFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC8FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC12FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC16FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC20FT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeNameTypeFT.h"

//mrf added 8-05
#include "Generic/descriptors/discmodel/featuretypes/FuncParentHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/NameModTypeHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last1LetterFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last2LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last3LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/RareHWListFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWBigramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWTrigramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/WindowGramFT.h"

//mrf last minute ACE additions
#include "Generic/descriptors/discmodel/featuretypes/AdjNameEntTypeFT.h"
#include "Generic/descriptors/discmodel/featuretypes/IdFWordFeatureFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWNextWordWCFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWPrevWordWCFT.h"

//tb 2007 ace additions
#include "Generic/descriptors/discmodel/featuretypes/HeadwordBigramWC8FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordBigramWC12FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordBigramWC16FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordBigramWC20FT.h"

#include "Generic/descriptors/discmodel/featuretypes/HeadwordNextWC8FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordNextWC12FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordNextWC16FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordNextWC20FT.h"

#include "Generic/descriptors/discmodel/featuretypes/WindowUniGramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWPosInSentFT.h"

bool ArabicP1DescFeatureTypes::_instantiated = false;

void ArabicP1DescFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a hash table in DTFeatureType

	_new HeadwordFT();
	_new FuncParentFT();
	_new PremodFT();
	_new PremodHeadFT();
	_new HeadwordWC8FT();
	_new HeadwordWC12FT();
	_new HeadwordWC16FT();
	_new HeadwordWC20FT();
	_new HeadwordBigramWC8FT();
	_new HeadwordBigramWC12FT();
	_new HeadwordBigramWC16FT();
	_new HeadwordBigramWC20FT();
	_new ModifyeeHeadFT(); // only for nom-premods
	_new ModifyeeNameTypeFT(); // only for nom-premods
	_new FuncParentHeadFT();
	_new NameModTypeHeadFT();
	_new Last1LetterFT();
	_new Last2LettersFT();
	_new Last3LettersFT();
	_new RareHWListFT();
	_new HWBigramFT();
	_new HWTrigramFT();
	_new WindowGramFT();
	_new WindowUniGramFT();
	_new AdjNameEntTypeFT();
	_new IdFWordFeatureFT();
	_new HWNextWordWCFT();
	_new HWPrevWordWCFT();
	_new HeadwordNextWC8FT();
	_new HeadwordNextWC12FT();
	_new HeadwordNextWC16FT();
	_new HeadwordNextWC20FT();
	_new HWPosInSentFT();
}
