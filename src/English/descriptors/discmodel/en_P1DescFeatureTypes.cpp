// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "English/descriptors/discmodel/en_P1DescFeatureTypes.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordFT.h"
#include "Generic/descriptors/discmodel/featuretypes/FuncParentFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodFT.h"
#include "Generic/descriptors/discmodel/featuretypes/PremodHeadFT.h"
#include "English/descriptors/discmodel/featuretypes/StemmedHeadwordFT.h"
#include "English/descriptors/discmodel/featuretypes/HeadwordWNFT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC8FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC12FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC16FT.h"
#include "Generic/descriptors/discmodel/featuretypes/HeadwordWC20FT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/ModifyeeNameTypeFT.h"

//mrf added 8-05
#include "Generic/descriptors/discmodel/featuretypes/FuncParentHeadFT.h"
#include "Generic/descriptors/discmodel/featuretypes/NameModTypeHeadFT.h"
#include "English/descriptors/discmodel/featuretypes/PropSubjFT.h"
#include "English/descriptors/discmodel/featuretypes/PropObjFT.h"
#include "English/descriptors/discmodel/featuretypes/PropIObjFT.h"
#include "English/descriptors/discmodel/featuretypes/PropCopulaFT.h"
#include "English/descriptors/discmodel/featuretypes/PrepFuncParentFT.h"
#include "English/descriptors/discmodel/featuretypes/NoParsePrepFuncParentFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last2LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/Last3LettersFT.h"
#include "Generic/descriptors/discmodel/featuretypes/RareHWListFT.h"

#include "Generic/descriptors/discmodel/featuretypes/AltModelDescPredictionFT.h"
#include "Generic/descriptors/discmodel/featuretypes/AltModelDescPredictionHWFT.h"

#include "English/descriptors/discmodel/featuretypes/NotInWordnetFT.h"
#include "English/descriptors/discmodel/featuretypes/PersonHyponymFT.h"
#include "English/descriptors/discmodel/featuretypes/NotPersonHyponymFT.h"

bool EnglishP1DescFeatureTypes::_instantiated = false;

void EnglishP1DescFeatureTypes::ensureFeatureTypesInstantiated() {
	if (_instantiated)
		return;
	_instantiated = true;

	// When these objects are created, they put themselves into
	// a hash table in DTFeatureType

	_new HeadwordFT();
	_new FuncParentFT();
	_new PremodFT();
	_new PremodHeadFT();
	_new EnglishStemmedHeadwordFT();
	_new HeadwordWC8FT();
	_new HeadwordWC12FT();
	_new HeadwordWC16FT();
	_new HeadwordWC20FT();
	_new EnglishHeadwordWNFT();
	_new ModifyeeHeadFT(); // only for nom-premods
	_new ModifyeeNameTypeFT(); // only for nom-premods

	_new FuncParentHeadFT();
	_new NameModTypeHeadFT();
	_new EnglishPropSubjFT();
	_new EnglishPropObjFT();
	_new EnglishPropIObjFT();
	_new EnglishPropCopulaFT();
	_new EnglishPrepFuncParentFT();
    _new EnglishNoParsePrepFuncParentFT();
	_new Last2LettersFT();
	_new Last3LettersFT();
	_new RareHWListFT();

	_new AltModelDescPredictionFT();
	_new AltModelDescPredictionHWFT();

	_new EnglishNotInWordnetFT();
	_new EnglishPersonHyponymFT();
	_new EnglishNotPersonHyponymFT();
}
