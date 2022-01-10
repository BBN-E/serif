// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TYPES_CLASH_AND_ENTITYLEVEL2_FT_H
#define TYPES_CLASH_AND_ENTITYLEVEL2_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

// DO not use this feature with pronoun as their type is not yet defined during coref
class TypesClashAndEntityLevel2FT : public DTCorefFeatureType {
public:
	TypesClashAndEntityLevel2FT() : DTCorefFeatureType(Symbol(L"types-clash-2-entity-lvl")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol
			, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol
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
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), mentType.getName(), SymbolConstants::nullSymbol, MATCH_SYM, entityMentionLevel);
		}else{
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), mentType.getName(), entType.getName(), CLASH_SYM, entityMentionLevel);
		}
		return 1;
	}

};
#endif
