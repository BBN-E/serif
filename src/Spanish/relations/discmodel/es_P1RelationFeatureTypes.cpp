// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Spanish/relations/es_RelationUtilities.h"

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

#include "Spanish/relations/discmodel/featuretypes/es_SimplePropFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_RolesTypesFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PrepStackFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePropTypesFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePropWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePropWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_NestedPropFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_RefPropFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_RefWithNameFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePropHWFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePropHWWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_HeadWordFT.h"
//#include "Spanish/relations/discmodel/featuretypes/es_SimplePropFNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PropTreeToplinkFT.h"

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
#include "Spanish/relations/discmodel/featuretypes/es_PossessiveRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PossessiveAfterMent2FT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PossessiveWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PossessiveWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PPRelWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePPRelWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_PPRelWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_SimplePPRelWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_VTypePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_NTypePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_VTypeSimplePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_NTypeSimplePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypeSimplePPRelFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypePPRelwithHeadFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_VerbPropFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_VerbPropWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_VerbPropWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_simpleVerbPropFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_simpleVerbPropwithTypeFT.h"

#include "Spanish/relations/discmodel/featuretypes/es_mixTypePPRelWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypePPRelWCFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypeSimplePPRelWNFT.h"
#include "Spanish/relations/discmodel/featuretypes/es_mixTypeSimplePPRelWCFT.h"

#include "Spanish/relations/discmodel/es_P1RelationFeatureTypes.h"

//bool SpanishP1RelationFeatureTypes::_instantiated = false;

SpanishP1RelationFeatureTypes::SpanishP1RelationFeatureTypes() {
	//if (_instantiated)
	//	return;
	//_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new EntityTypesFT();
	_new EntityTypesPlusSubtypesFT();
	_new WBJustTypesFT();
	_new SpanishRolesTypesFT();
	_new NegativePropFT();
	_new SpanishSimplePropFT();
	//_new SpanishSimplePropFNFT(); //test
	_new SpanishPropTreeToplinkFT(); //new!
	_new SpanishNestedPropFT();
	_new SpanishSimplePropWCFT();
	_new SpanishSimplePropWNFT();
	_new SpanishRefPropFT();
	_new SpanishRefWithNameFT();
	_new WordsBetweenTypesFT();
	_new POSBetweenTypesFT();
	_new WordsBetweenCondTypesFT();
	_new SpanishSimplePropTypesFT();
	_new EntityPlusMentionTypesFT();
	_new SpanishPrepStackFT();
	_new StemmedWBTypesFT();
	_new PriorFT();
	_new SpanishSimplePropHWFT();
	_new SpanishSimplePropHWWCFT();
	_new SpanishHeadWordFT();
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
	_new SpanishPossessiveRelFT();
	_new SpanishPossessiveAfterMent2FT();
	_new SpanishPossessiveWNFT();
	_new SpanishPossessiveWCFT();
	_new SpanishPPRelFT();
	_new SpanishSimplePPRelFT();
	_new SpanishPPRelWNFT();
	_new SpanishSimplePPRelWNFT();
	_new SpanishPPRelWCFT();
	_new SpanishSimplePPRelWCFT();
	_new SpanishNTypePPRelFT();
	_new SpanishVTypePPRelFT();
	_new SpanishMixTypePPRelFT();
	_new SpanishNTypeSimplePPRelFT();
	_new SpanishVTypeSimplePPRelFT();
	_new SpanishMixTypeSimplePPRelFT();
	_new SpanishMixTypePPRelwithHeadFT();
	_new SpanishVerbPropFT();
	_new SpanishVerbPropWCFT();
	_new SpanishVerbPropWNFT();
	_new SpanishSimpleVerbPropFT();
	_new SpanishSimpleVerbPropwithTypeFT();


	_new SpanishMixTypePPRelWNFT();
	_new SpanishMixTypePPRelWCFT();
	_new SpanishMixTypeSimplePPRelWNFT();
	_new SpanishMixTypeSimplePPRelWCFT();
}
