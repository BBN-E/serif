// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTFeatureTypeSet.h"

// This is where the _new instances of the feature types live.

#include "Generic/edt/discmodel/DTCorefFeatureTypes.h"
#include "Generic/edt/discmodel/xx_DTCorefFeatureTypes.h"

#include "Generic/edt/HobbsDistance.h"

#include "Generic/edt/discmodel/featuretypes/PreLinkFT.h"
#include "Generic/edt/discmodel/featuretypes/CommonEntityLinkFT.h"
#include "Generic/edt/discmodel/featuretypes/UniqueModRatioFT.h"
#include "Generic/edt/discmodel/featuretypes/UniqueCharRatioFT.h"
#include "Generic/edt/discmodel/featuretypes/DistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/DistanceWideFT.h"
#include "Generic/edt/discmodel/featuretypes/SentDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/ClosestMentDistance4FT.h"
#include "Generic/edt/discmodel/featuretypes/ClosestMentDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/EntHasNumericModFT.h"
#include "Generic/edt/discmodel/featuretypes/NumEntByTypeFT.h"
#include "Generic/edt/discmodel/featuretypes/NumEntByType2FT.h"
#include "Generic/edt/discmodel/featuretypes/EntityTypeFT.h"
#include "Generic/edt/discmodel/featuretypes/HasModClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HasModMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HasNameModClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HasNameModMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HasNumericClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HasNumericMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWClusterAgreementFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/LastCharMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchAllFT.h"

#include "Generic/edt/discmodel/featuretypes/StringMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesFT.h"
#include "Generic/edt/discmodel/featuretypes/OverlappingFT.h"
#include "Generic/edt/discmodel/featuretypes/OverlappingHWFT.h"
#include "Generic/edt/discmodel/featuretypes/MentIncludedFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchAndModNameMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchAndModNameClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchAndModNameMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchAndModNameClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchAndNumericMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchAndNumericClashFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchAndNumericMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchAndNumericClashFT.h"

#include "Generic/edt/discmodel/featuretypes/ModClashAndUMRatioFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNumericAndEntNumericFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNumericAndEntNumericAndMatchAndClashFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNumericAndEntNumericAndClashFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNumericAndEntNumericAndMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesAndModNameClashFT.h"

#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesAndEntTypeFT.h"
#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesAndMentNoModsFT.h"
#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesAndModNameMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/EntOnlyNamesAndUMRatioFT.h"
#include "Generic/edt/discmodel/featuretypes/ModClashAndModMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/NotEntNumericMentNumericFT.h"
#include "Generic/edt/discmodel/featuretypes/EntNumericNotMentNumericFT.h"
#include "Generic/edt/discmodel/featuretypes/HWNodeMatchAndUMRatioFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHWMatchEntNameWordFT.h"

#include "Generic/edt/discmodel/featuretypes/EntModsFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHWEntHWFT.h"
#include "Generic/edt/discmodel/featuretypes/MentModEntModFT.h"
#include "Generic/edt/discmodel/featuretypes/MentClustEntClustFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHWEntModFT.h"
#include "Generic/edt/discmodel/featuretypes/EntityLevelFT.h"

#include "Generic/edt/discmodel/featuretypes/HWMatchGPEModMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/P1DLSubtypesMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/P1DLSubtypesClash.h"
#include "Generic/edt/discmodel/featuretypes/P1DLMentSubtypeEntHW.h"
#include "Generic/edt/discmodel/featuretypes/P1DLEntSubtypeMentHW.h"
#include "Generic/edt/discmodel/featuretypes/P1DLSubtype.h"
#include "Generic/edt/discmodel/featuretypes/P1DLPOSBtwnFT.h"
#include "Generic/edt/discmodel/featuretypes/P1DLPOSBtwnPrtFT.h"
#include "Generic/edt/discmodel/featuretypes/HobbsDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/HWandHobbsDistanceFT.h"

#include "Generic/edt/discmodel/featuretypes/PLTypeParentWordFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeParentWordHWFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeParentWordWCFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeNumberGenderFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeNumberGenderHWFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeHobbsDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeMentHWFT.h"
#include "Generic/edt/discmodel/featuretypes/PLTypeHobbsDistanceHWFT.h"

#include "Generic/edt/discmodel/featuretypes/EntHasHWFT.h"
#include "Generic/edt/discmodel/featuretypes/EntHasHWHWFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHWEntLastPronFT.h"
#include "Generic/edt/discmodel/featuretypes/HWandDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/TypesClashFT.h"

#include "Generic/edt/discmodel/featuretypes/NMExactAbbrevMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/NMMentTypesMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/AcronymMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/FullMentMentEditDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/FullMentMentEditDistance2FT.h"
#include "Generic/edt/discmodel/featuretypes/FullMentMentEditDistance3FT.h"
#include "Generic/edt/discmodel/featuretypes/MentMentEditDistanceFT.h"
#include "Generic/edt/discmodel/featuretypes/MentMentEditDistance2FT.h"
#include "Generic/edt/discmodel/featuretypes/MentMentEditDistance3FT.h"
#include "Generic/edt/discmodel/featuretypes/PremodHWMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/PostmodHWMatchFT.h"
#include "Generic/edt/discmodel/featuretypes/HWMatchHWFT.h"
#include "Generic/edt/discmodel/featuretypes/SentDistanceAndLevelFT.h"
#include "Generic/edt/discmodel/featuretypes/TypesClashAndEntityLevelFT.h"
#include "Generic/edt/discmodel/featuretypes/TypesClashAndEntityLevel2FT.h"
#include "Generic/edt/discmodel/featuretypes/P1DLSubtypesMatchAndTypeFT.h"
#include "Generic/edt/discmodel/featuretypes/SubypesClashAndEntityLevelFT.h"
#include "Generic/edt/discmodel/featuretypes/WithinBracketsHWMatchFT.h"

// mention-only or entity-set only FT
#include "Generic/edt/discmodel/featuretypes/MentHWFT.h"
#include "Generic/edt/discmodel/featuretypes/MentOverlappingFT.h"
#include "Generic/edt/discmodel/featuretypes/MentSentenceNumberFT.h"
#include "Generic/edt/discmodel/featuretypes/MentPositionInSentenceFT.h"
#include "Generic/edt/discmodel/featuretypes/NumEntFT.h"
#include "Generic/edt/discmodel/featuretypes/MentSynParentFT.h"
#include "Generic/edt/discmodel/featuretypes/MentModsFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHasNumericModFT.h"
#include "Generic/edt/discmodel/featuretypes/MentHasNameModFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNoModsFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNTerminalsFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNHeadTerminalsFT.h"
#include "Generic/edt/discmodel/featuretypes/MentTypeFT.h"
#include "Generic/edt/discmodel/featuretypes/MentGenderFT.h"
#include "Generic/edt/discmodel/featuretypes/MentNumberFT.h"
#include "Generic/edt/discmodel/featuretypes/MentMetonymicFT.h"
//mrf
#include "Generic/edt/discmodel/featuretypes/WorldKnowledgeFT.h"
//pam -- tested and found lacking
//#include "Generic/edt/discmodel/featuretypes/AlwaysTrueFT.h"
//#include "Generic/edt/discmodel/featuretypes/PositiveBiasFT.h"


// THESE ARE RELATION DEPENDENT FEATURE_TYPES
// FOR THEM TO WORK WE NEED TO UNCOMMENT SOME LINES IN THE DT_COREF_OBSERVATION
//#include "Generic/edt/discmodel/featuretypes/MentHWEntityCoRelationFT.h"
//#include "Generic/edt/discmodel/featuretypes/MentHWEntityCoRelationSameHeadFT.h"
//#include "Generic/edt/discmodel/featuretypes/MentRelationsFT.h"
//#include "Generic/edt/discmodel/featuretypes/EntityRelationsFT.h"
//#include "Generic/edt/discmodel/featuretypes/SameRelMentionFT.h"

//bool DTCorefFeatureTypes::_instantiated = false;
bool DTCorefFeatureTypes::_instantiated = false;

//void DTCorefFeatureTypes::ensureFeatureTypesInstantiated() {
void DTCorefFeatureTypes::ensureBaseFeatureTypesInstantiated(){
	if (_instantiated)
		return;
	_instantiated = true;

	Guesser::initialize();
	HobbsDistance::initialize();

	// When these objects are created, they put themselves into
	// a _new hash table in DTFeatureType
	_new HobbsDistanceFT();
	_new PreLinkFT();
	_new CommonEntityLinkFT();

	_new P1DLSubtypesMatchFT();
	_new P1DLSubtypesClashFT();
	_new P1DLMentSubtypeEntHWFT();
	_new P1DLEntSubtypeMentHWFT();
	_new P1DLSubtypeFT();
	_new P1DLPOSBtwnFT();
	_new P1DLPOSBtwnPrtFT();
	_new P1DLSubtypesMatchAndTypeFT();
	_new SubypesClashAndEntityLevelFT();

	_new ModClashAndUMRatioFT();
	_new MentNumericAndEntNumericFT();
	_new MentNumericAndEntNumericAndMatchAndClashFT();
	_new MentNumericAndEntNumericAndClashFT();
	_new MentNumericAndEntNumericAndMatchFT();
	_new EntOnlyNamesAndModNameClashFT();
	_new EntOnlyNamesAndEntTypeFT();
	_new EntOnlyNamesMentNoModsFT();
	_new EntOnlyNamesModNameMatchFT();
	_new EntOnlyNamesAndUMRatioFT();
	_new NotEntNumericAndMentNumericFT();
	_new EntNumericAndNotMentNumericFT();
	_new ModClashAndModMatchFT();
	_new HWNodeMatchAndUMRatioFT();

	_new StringMatchFT();
	_new EntOnlyNamesFT();
	_new OverlappingFT();
	_new OverlappingHWFT();
	_new MentIncludedFT();
	_new HWNodeMatchAndModNameMatchFT();
	_new HWNodeMatchAndModNameClashFT();
	_new HWMatchAndModNameMatchFT();
	_new HWMatchAndModNameClashFT();
	_new HWNodeMatchAndNumericMatchFT();
	_new HWNodeMatchAndNumericClashFT();
	_new HWMatchAndNumericMatchFT();
	_new HWMatchAndNumericClashFT();
	_new MentHWMatchEntNameWordFT();

	_new UniqueModRatioFT();
	_new UniqueCharRatioFT();
	_new DistanceFT();
	_new DistanceWideFT();
	_new SentDistanceFT(); // same as DistanceFT but different string identifier for backward compliance
	_new ClosestMentDistance4FT(); // deprecated ClosestMentDistanceFT
	_new ClosestMentDistanceFT(); 
	_new EntHasNumericModFT();
	_new NumEntByTypeFT();
	_new NumEntByType2FT();
	_new EntityTypeFT();
	_new HasModClashFT();
	_new HasModMatchFT();
	_new HasNameModClashFT();
	_new HasNameModMatchFT();
	_new HasNumericClashFT();
	_new HasNumericMatchFT();
	_new HWClusterAgreementFT();
	_new HWMatchFT();
	_new LastCharMatchFT();
	_new HWNodeMatchFT();

	_new EntModsFT();
	_new MentHWEntHWFT();
	_new MentModEntModFT();
	_new MentClustEntClustFT();
	_new MentHWEntModFT();
	_new HWMatchGPEModMatchFT();
	_new HWandHobbsDistanceFT();

	_new PLTypeMentHWFT();
	_new PLTypeParentWordFT();
	_new PLTypeParentWordHWFT();
	_new PLTypeParentWordWCFT();
	_new PLTypeNumberGenderFT();
	_new PLTypeNumberGenderHWFT();
	_new PLTypeHobbsDistanceFT();

	_new PLTypeHobbsDistanceHWFT();
	_new EntHasHWFT();
	_new EntHasHWHWFT();
	_new MentHWEntLastPronFT();
	_new HWandDistanceFT();
	_new TypesClashFT();

	// For Name linking
	_new HWMatchAllFT();
	_new EntityLevelFT();
	_new NMExactAbbrevMatchFT();
	_new NMMentTypesMatchFT();
	_new AcronymMatchFT();
	_new MentMentEditDistanceFT();
	_new MentMentEditDistance2FT();
	_new MentMentEditDistance3FT();
	_new FullMentMentEditDistanceFT();
	_new FullMentMentEditDistance2FT();
	_new FullMentMentEditDistance3FT();
	_new PremodHWMatchFT();
	_new PostmodHWMatchFT();
	_new HWMatchHWFT();
	_new SentDistanceAndLevelFT();
	_new TypesClashAndEntityLevelFT();
	_new TypesClashAndEntityLevel2FT();
	_new WithinBracketsHWMatchFT();

	// MENTION DEPENDENT ONLY FEATURES
	_new MentHWFT();
	_new MentOverlappingFT();
	_new MentSentenceNumberFT();
	_new MentPositionInSentenceFT();
	_new NumEntFT();
	_new MentSynParentFT();
	_new MentModsFT();
	_new MentHasNameModFT();
	_new MentHasNumericModFT();
	_new MentNoModsFT();
	// For Name linking
	_new MentNTerminalsFT();
	_new MentNHeadTerminalsFT();
	_new MentTypeFT();
	_new MentGenderFT();
	_new MentNumberFT();
	_new MentMetonymicFT();

// THESE ARE RELATION DEPENDENT FEATURE_TYPES
// FOR THEM TO WORK WE NEED TO UNCOMMENT SOME LINES IN THE DT_COREF_OBSERVATION
//	_new MentHWEntityCoRelationFT();
//	_new MentHWEntityCoRelationSameHeadFT();
//	_new MentRelationsFT();
//	_new EntityRelationsFT();
//	_new SameRelMentionFT();
	
	_new WorldKnowledgeFT();
// These two showed promise in combination with world knowledge but did not  improve final version
	//_new PositiveBiasFT();
	//_new AlwaysTrueFT();

}

DTFeatureTypeSet* DTCorefFeatureTypes::makeNoneFeatureTypeSet(Mention::Type type) {
	DTFeatureTypeSet *result = 0;
	if (type == Mention::NAME) {
		result = _new DTFeatureTypeSet(7);
//		result = _new DTFeatureTypeSet(8);
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-hw"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-sent-num"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-overlapping-ment"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"num-entities"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-num-terminals"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-num-head-terminals"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-ent-type"));			
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-gender"));			
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-number"));			
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-metonymic"));	
	} else if (type == Mention::DESC) {
		if (SerifVersion::isEnglish()) {
			result = _new DTFeatureTypeSet(10);
			result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"is-ment-generic"));
		} else {
			result = _new DTFeatureTypeSet(9);
		}
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-hw"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-sent-num"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-overlapping-ment"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-pos-in-sent"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-mods"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-has-numeric-mod"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-has-name-mod"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-no-mods"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"num-entities"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-possesive"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-syn-parent"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-ent-type"));			
	} else if (type == Mention::PRON) {
//		result = _new DTFeatureTypeSet(1);
//		result = _new DTFeatureTypeSet(6);
		result = _new DTFeatureTypeSet(4);
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-hw"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-sent-num"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-overlapping-ment"));
		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-pos-in-sent"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"num-entities"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-possesive"));
//		result->addFeatureType(DTCorefFeatureType::modeltype,Symbol(L"ment-syn-parent"));
	}
	return result;
}

boost::shared_ptr<DTCorefFeatureTypes::Factory> &DTCorefFeatureTypes::_factory() {
	static boost::shared_ptr<DTCorefFeatureTypes::Factory> factory(new GenericDTCorefFeatureTypesFactory());
	return factory;
}

