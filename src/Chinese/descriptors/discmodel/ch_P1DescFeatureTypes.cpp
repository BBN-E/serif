// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Chinese/descriptors/discmodel/ch_P1DescFeatureTypes.h"

#include "Generic/descriptors/discmodel/P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordFT.h"
#include "Generic/descriptors/discmodel/featuretypes/FuncParentFT.h"
#include "Generic/descriptors/discmodel/featuretypes/FuncParentHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC8FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC12FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC16FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC20FT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeNameTypeFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWBigramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWTrigramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last2LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last3LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/NameModTypeHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/RareHWListFT.h"
#include "Generic/descriptors/discmodel/featuretypes/WindowGramFT.h"
#include "Generic/descriptors/discmodel/featuretypes/AdjNameEntTypeFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWNextWordWCFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HWPrevWordWCFT.h"
#include "Generic/descriptors/discmodel/featuretypes/IdFWordFeatureFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/LastCharFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/ContainsCharFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/PrepFuncParentFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/PropCopulaFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/PropIObjFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/PropObjFT.h"
#include "Chinese/descriptors/discmodel/featuretypes/PropSubjFT.h"


bool ChineseP1DescFeatureTypes::_instantiated = false;

void ChineseP1DescFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new HeadwordFT();
	_new FuncParentFT();
	_new FuncParentHeadFT();
	_new PremodFT();
	_new PremodHeadFT();
	_new HeadwordWC8FT();
	_new HeadwordWC12FT();
	_new HeadwordWC16FT();
	_new HeadwordWC20FT();
	_new ModifyeeHeadFT();
	_new ModifyeeNameTypeFT();
	_new HWBigramFT();
	_new HWTrigramFT();
	_new Last2LettersFT();
	_new Last3LettersFT();
	_new NameModTypeHeadFT();
	_new RareHWListFT();
	_new WindowGramFT();
	_new AdjNameEntTypeFT();
	_new HWNextWordWCFT();
	_new HWPrevWordWCFT();
	_new IdFWordFeatureFT();
	_new ChineseLastCharFT();
	_new ChineseContainsCharFT();
	_new ChinesePrepFuncParentFT();
	_new ChinesePropCopulaFT();
	_new ChinesePropIObjFT();
	_new ChinesePropObjFT();
	_new ChinesePropSubjFT();

}
