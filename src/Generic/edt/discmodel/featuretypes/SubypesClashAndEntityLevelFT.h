// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SUBTYPES_CLASH_AND_ENTITYLEVEL_FT_H
#define SUBTYPES_CLASH_AND_ENTITYLEVEL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/EntitySet.h"
#include "Generic/theories/EntitySubtype.h"
#include "Generic/theories/Mention.h"

// DO not use this feature with pronoun as their type is not yet defined during coref
class SubypesClashAndEntityLevelFT : public DTCorefFeatureType {
public:
	SubypesClashAndEntityLevelFT() : DTCorefFeatureType(Symbol(L"subtypes-match-clash-entity-lvl")) {
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
		const Mention *ment = o->getMention();
		const Entity *entity = o->getEntity();
		const EntityType mentType = ment->getEntityType();
		const EntityType entType = entity->getType();
		if(!mentType.isDetermined() || !entType.isDetermined())
			return 0;
		if(mentType != entType)
			return 0;

		Symbol entityMentionLevel = o->getEntityMentionLevel();

		const EntitySet *eset = o->getEntitySet();
		EntitySubtype entSubtype = eset->guessEntitySubtype(entity);

		EntitySubtype mentSubtype = ment->getEntitySubtype();
		if(!mentSubtype.isDetermined() && !entSubtype.isDetermined())
			return 0;

		if(mentType == entType){
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), mentSubtype.getName(), SymbolConstants::nullSymbol, MATCH_SYM, entityMentionLevel);
		}else{
				resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), mentSubtype.getName(), entSubtype.getName(), CLASH_SYM, entityMentionLevel);
		}
		return 1;
	}

};
#endif
