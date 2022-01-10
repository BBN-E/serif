// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TYPES_CLASH_AND_ENTITYLEVEL_FT_H
#define TYPES_CLASH_AND_ENTITYLEVEL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

// DO not use this feature with pronoun as their type is not yet defined during coref
class TypesClashAndEntityLevelFT : public DTCorefFeatureType {
//	Symbol  MATCH_OTHER_SYM;
public:
	TypesClashAndEntityLevelFT() : DTCorefFeatureType(Symbol(L"types-clash-entity-lvl")) {
//		MATCH_OTHER_SYM = Symbol(L"MATCH_OTH");
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		EntityType mentType = o->getMention()->getEntityType();
		EntityType entType = o->getEntity()->getType();
		if(!mentType.isDetermined() || !entType.isDetermined()){
			return 0;
		}

		Symbol entityMentionLevel = o->getEntityMentionLevel();

		if(mentType == entType){
//			if(mentType == EntityType::getOtherType() || mentType == EntityType::getUndetType())
//				resultArray[0] = _new DTBigramFeature(this, state.getTag(), MATCH_OTHER_SYM);
//			else
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(), MATCH_SYM, entityMentionLevel);
		}else{
				resultArray[0] = _new DTTrigramFeature(this, state.getTag(), CLASH_SYM, entityMentionLevel);
		}
		return 1;
	}

};
#endif
