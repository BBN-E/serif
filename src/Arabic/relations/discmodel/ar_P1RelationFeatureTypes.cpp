// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

// This is where the _new instances of the feature types live.

#include "Generic/relations/discmodel/featuretypes/EntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/EntityPlusMentionTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/MentionDistanceFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_HeadWordWCFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LastPrepFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_PrepChainFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_POSBetweenTypesFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_HeadWordFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftHeadWordFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightHeadWordFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_EntityAndMentionTypesFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftCliticFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightCliticFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftIsDefiniteFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightIsDefiniteFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftIsModifierFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightIsModifierFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftEntityAndMentionTypesFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightEntityAndMentionTypesFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LastPrepNPrepsFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_MentionChunkDistanceFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_LeftHeadWordWCFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_RightHeadWordWCFT.h"

//parse based features
#include "Generic/relations/discmodel/featuretypes/ParsePPModFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistFT.h"
#include "Generic/relations/discmodel/featuretypes/HWCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/SharedAncestorAndDistAndClustFT.h"
#include "Generic/relations/discmodel/featuretypes/HWClustCommonAncestFT.h"
#include "Generic/relations/discmodel/featuretypes/ParsePathBetweenFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_PossModFT.h"
#include "Generic/relations/discmodel/featuretypes/NomModFT.h"

#include "Generic/relations/discmodel/featuretypes/EntityTypesPlusSubtypesFT.h"
#include "Generic/relations/discmodel/featuretypes/WordsBetweenCondTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypesFT.h"
#include "Generic/relations/discmodel/featuretypes/AltModelPredictionEntityTypeSubtypeFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_PossPronFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_AdjPOSFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_AdjModEntTypeHWFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_PrepSepEntTypeHWFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_PrepSepEntTypeClustFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_AdjModEntTypeClustFT.h"

#include "Arabic/relations/discmodel/featuretypes/ar_PossPronClustFT.h"
#include "Arabic/relations/discmodel/featuretypes/ar_ParenGPEFT.h"
#include "Arabic/relations/discmodel/ar_P1RelationFeatureTypes.h"


#include "Generic/common/ParamReader.h"
#include "Arabic/relations/ar_RelationUtilities.h"

//bool ArabicP1RelationFeatureTypes::_instantiated = false;

ArabicP1RelationFeatureTypes::ArabicP1RelationFeatureTypes() {
	//if (_instantiated)
	//	return;
	//_instantiated = true;

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType

	_new EntityTypesFT();
	_new WordsBetweenTypesFT();
	_new EntityPlusMentionTypesFT();
	_new ArabicHeadWordWCFT();
	_new ArabicLastPrepFT();
	_new ArabicPrepChainFT();
	_new MentionDistanceFT();
	_new ArabicPOSBetweenTypesFT();
	_new ArabicHeadWordFT();
	_new ArabicLeftHeadWordFT();
	_new ArabicRightHeadWordFT();
	_new ArabicEntityAndMentionTypesFT();
	_new ArabicLeftCliticFT();
	_new ArabicRightCliticFT();
	_new ArabicLeftIsDefiniteFT();
	_new ArabicRightIsDefiniteFT();
	_new ArabicLeftIsModifierFT();
	_new ArabicRightIsModifierFT();
	_new ArabicLeftEntityAndMentionTypesFT();
	_new ArabicRightEntityAndMentionTypesFT();
	_new ArabicLastPrepNPrepsFT();
	_new ArabicMentionChunkDistanceFT();
	_new ArabicLeftHeadWordWCFT();
	_new ArabicRightHeadWordWCFT();
	_new ParsePPModFT();
	_new SharedAncestorAndDistFT();
	
	_new HWCommonAncestFT();
	_new SharedAncestorAndDistAndClustFT();
	_new HWClustCommonAncestFT();
	_new ParsePathBetweenFT();
	_new ArabicPossModFT();
	_new NomModFT();
	
	_new EntityTypesPlusSubtypesFT();
	_new WordsBetweenCondTypesFT();

	_new AltModelPredictionFT();
	_new AltModelPredictionEntityTypesFT();
	_new AltModelPredictionEntityTypeSubtypeFT();

	_new ArabicPossPronFT();
	_new ArabicAdjPOSFT();
	_new ArabicAdjModEntTypeHWFT();
	_new ArabicPrepSepEntTypeHWFT();
	_new ArabicPrepSepEntTypeClustFT();
	_new ArabicAdjModEntTypeClustFT();

	//last minute ACE05 features
	_new ArabicPossPronClustFT();
	_new ArabicParenGPEFT();

}
