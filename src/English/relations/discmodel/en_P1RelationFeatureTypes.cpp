// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "English/relations/en_RelationUtilities.h"

// This is where the _new instances of the feature types live.

#include "Generic/relations/discmodel/featuretypes/EntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/EntityTypesPlusSubtypesFT.h"
#include "Generic/relations/discmodel/featuretypes/EntityPlusMentionTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/POSBetweenTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/StemmedWBTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenCondTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/NegativePropFT.h"
#include "Generic/relations/discmodel/featuretypes/WBJustTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/PriorFT.h"
#include "Generic/relations/discmodel/featuretypes/HeadWordWCFT.h"
#include "Generic/relations/discmodel/featuretypes/DocTopicFeatureType.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypeSubtypeFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/XDocFT.h"

#include "English/relations/discmodel/featuretypes/en_SimplePropFT.h"
#include "English/relations/discmodel/featuretypes/en_RolesTypesFT.h"
#include "English/relations/discmodel/featuretypes/en_PrepStackFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePropTypesFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePropWCFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePropWNFT.h"
#include "English/relations/discmodel/featuretypes/en_NestedPropFT.h"
#include "English/relations/discmodel/featuretypes/en_RefPropFT.h"
#include "English/relations/discmodel/featuretypes/en_RefWithNameFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePropHWFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePropHWWCFT.h"
#include "English/relations/discmodel/featuretypes/en_HeadWordFT.h"
//#include "English/relations/discmodel/featuretypes/en_SimplePropFNFT.h"
#include "English/relations/discmodel/featuretypes/en_PropTreeToplinkFT.h"

//parse based features
#include "Generic/relations/discmodel/featuretypes/ParsePPModFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistFT.h"
#include "Generic/relations/discmodel/featuretypes/HWCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistAndClustFT.h"
#include "Generic/relations/discmodel/featuretypes/HWClustCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/ParsePathBetweenFT.h"
#include "Generic/relations/discmodel/featuretypes/NomModFT.h"


// added after 12/05/2007  for NP chunk based relation models
#include "Generic/relations/discmodel/featuretypes/EntityPlusMentionTypesRelaxedFT.h"
#include "English/relations/discmodel/featuretypes/en_PossessiveRelFT.h"
#include "English/relations/discmodel/featuretypes/en_PossessiveAfterMent2FT.h"
#include "English/relations/discmodel/featuretypes/en_PossessiveWNFT.h"
#include "English/relations/discmodel/featuretypes/en_PossessiveWCFT.h"
#include "English/relations/discmodel/featuretypes/en_PPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_PPRelWNFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePPRelWNFT.h"
#include "English/relations/discmodel/featuretypes/en_PPRelWCFT.h"
#include "English/relations/discmodel/featuretypes/en_SimplePPRelWCFT.h"
#include "English/relations/discmodel/featuretypes/en_VTypePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_NTypePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_VTypeSimplePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_NTypeSimplePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypeSimplePPRelFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypePPRelwithHeadFT.h"
#include "English/relations/discmodel/featuretypes/en_VerbPropFT.h"
#include "English/relations/discmodel/featuretypes/en_VerbPropWNFT.h"
#include "English/relations/discmodel/featuretypes/en_VerbPropWCFT.h"
#include "English/relations/discmodel/featuretypes/en_simpleVerbPropFT.h"
#include "English/relations/discmodel/featuretypes/en_simpleVerbPropwithTypeFT.h"

#include "English/relations/discmodel/featuretypes/en_mixTypePPRelWNFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypePPRelWCFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypeSimplePPRelWNFT.h"
#include "English/relations/discmodel/featuretypes/en_mixTypeSimplePPRelWCFT.h"

#include "English/relations/discmodel/en_P1RelationFeatureTypes.h"

//bool EnglishP1RelationFeatureTypes::_instantiated = false;

EnglishP1RelationFeatureTypes::EnglishP1RelationFeatureTypes() {
	//if (_instantiated)
	//	return;
	//_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new EntityTypesFT();
	_new EntityTypesPlusSubtypesFT();
	_new WBJustTypesFT();
	_new EnglishRolesTypesFT();
	_new NegativePropFT();
	_new EnglishSimplePropFT();
	//_new EnglishSimplePropFNFT(); //test
	_new EnglishPropTreeToplinkFT(); //new!
	_new EnglishNestedPropFT();
	_new EnglishSimplePropWCFT();
	_new EnglishSimplePropWNFT();
	_new EnglishRefPropFT();
	_new EnglishRefWithNameFT();
	_new WordsBetweenTypesFT();
	_new POSBetweenTypesFT();
	_new WordsBetweenCondTypesFT();
	_new EnglishSimplePropTypesFT();
	_new EntityPlusMentionTypesFT();
	_new EnglishPrepStackFT();
	_new StemmedWBTypesFT();
	_new PriorFT();
	_new EnglishSimplePropHWFT();
	_new EnglishSimplePropHWWCFT();
	_new EnglishHeadWordFT();
	_new HeadWordWCFT();
	_new DocTopicFeatureType();
	_new XDocFT();

	_new HWCommonAncestFT();
	_new SharedAncestorAndDistAndClustFT();
	_new SharedAncestorAndDistFT();
	_new HWClustCommonAncestFT();
	_new ParsePathBetweenFT();
	_new ParsePPModFT();
	_new NomModFT();

	_new AltModelPredictionFT();
	_new AltModelPredictionEntityTypeSubtypeFT();
	_new AltModelPredictionEntityTypesFT();

	// added after 12/05/2007  for NP chunk based relation models
	_new EntityPlusMentionTypesRelaxedFT();
	_new EnglishPossessiveRelFT();
	_new EnglishPossessiveAfterMent2FT();
	_new EnglishPossessiveWNFT();
	_new EnglishPossessiveWCFT();
	_new EnglishPPRelFT();
	_new EnglishSimplePPRelFT();
	_new EnglishPPRelWNFT();
	_new EnglishSimplePPRelWNFT();
	_new EnglishPPRelWCFT();
	_new EnglishSimplePPRelWCFT();
	_new EnglishNTypePPRelFT();
	_new EnglishVTypePPRelFT();
	_new EnglishMixTypePPRelFT();
	_new EnglishNTypeSimplePPRelFT();
	_new EnglishVTypeSimplePPRelFT();
	_new EnglishMixTypeSimplePPRelFT();
	_new EnglishMixTypePPRelwithHeadFT();
	_new EnglishVerbPropFT();
	_new EnglishVerbPropWCFT();
	_new EnglishVerbPropWNFT();
	_new EnglishSimpleVerbPropFT();
	_new EnglishSimpleVerbPropwithTypeFT();


	_new EnglishMixTypePPRelWNFT();
	_new EnglishMixTypePPRelWCFT();
	_new EnglishMixTypeSimplePPRelWNFT();
	_new EnglishMixTypeSimplePPRelWCFT();
}
