// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/parse/ch_STags.h"

// This is where the _new instances of the feature types live.

#include "Generic/relations/discmodel/featuretypes/EntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/EntityPlusMentionTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenCondTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/POSBetweenTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/NegativePropFT.h"
#include "Generic/relations/discmodel/featuretypes/MentionDistanceFT.h"
#include "Generic/relations/discmodel/featuretypes/PriorFT.h"
#include "Generic/relations/discmodel/featuretypes/HeadWordFT.h"
#include "Generic/relations/discmodel/featuretypes/HeadWordWCFT.h"
#include "Generic/relations/discmodel/featuretypes/EntityTypesPlusSubtypesFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypeSubtypeFT.h"
#include "Generic/relations/discmodel/featuretypes/HWClustCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/HWCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/NomModFT.h"
#include "Generic/relations/discmodel/featuretypes/ParsePathBetweenFT.h"
#include "Generic/relations/discmodel/featuretypes/ParsePPModFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistAndClustFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistFT.h"
#include "Generic/relations/discmodel/featuretypes/StemmedWBTypesFT.h"
/*#include "Generic/relations/discmodel/featuretypes/MentionTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/LeftEntityTypeFT.h"
#include "Generic/relations/discmodel/featuretypes/RightEntityTypeFT.h"
#include "Generic/relations/discmodel/featuretypes/LeftMentionTypeFT.h"
#include "Generic/relations/discmodel/featuretypes/RightMentionTypeFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenFT.h"
#include "Generic/relations/discmodel/featuretypes/POSBetweenFT.h"
#include "Generic/relations/discmodel/featuretypes/WCFT.h"*/
#include "Chinese/relations/discmodel/featuretypes/ch_SimplePropFT.h"
#include "Chinese/relations/discmodel/featuretypes/ch_SimplePropTypesFT.h"
#include "Chinese/relations/discmodel/featuretypes/ch_SimplePropWCFT.h"
#include "Chinese/relations/discmodel/featuretypes/ch_NestedPropFT.h"
#include "Chinese/relations/discmodel/featuretypes/ch_RefPropFT.h"
//#include "Chinese/relations/discmodel/featuretypes/ch_SimplePropWCNoTypesFT.h"


#include "Chinese/relations/discmodel/ch_P1RelationFeatureTypes.h"

//bool ChineseP1RelationFeatureTypes::_instantiated = false;

ChineseP1RelationFeatureTypes::ChineseP1RelationFeatureTypes() {
	//if (_instantiated)
	//	return;
	//_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new EntityTypesFT();
	_new NegativePropFT();
	_new ChineseSimplePropFT();
	_new ChineseNestedPropFT();
	_new ChineseSimplePropWCFT();
	_new ChineseRefPropFT();
	_new WordsBetweenTypesFT();
	_new WordsBetweenCondTypesFT();
	_new POSBetweenTypesFT();
	_new ChineseSimplePropTypesFT();
	_new EntityPlusMentionTypesFT();
	_new MentionDistanceFT();
	_new PriorFT();
	_new HeadWordFT();
	_new HeadWordWCFT();
	_new EntityTypesPlusSubtypesFT();
	_new AltModelPredictionFT();
	_new AltModelPredictionEntityTypesFT();
	_new AltModelPredictionEntityTypeSubtypeFT();
	_new HWClustCommonAncestFT();
	_new HWCommonAncestFT();
	_new NomModFT();
	_new ParsePathBetweenFT();
	_new ParsePPModFT();
	_new SharedAncestorAndDistAndClustFT();
	_new SharedAncestorAndDistFT();
	_new StemmedWBTypesFT();
	/*_new LeftEntityTypeFT();
	_new RightEntityTypeFT();
	_new LeftMentionTypeFT();
	_new RightMentionTypeFT();
	_new PosBetweenFT();
	_new WordsBetweenFT();
	_new WCFT();
	_new ChineseSimplePropWCNoTypesFT();
	_new MentionTypesFT();*/
}
