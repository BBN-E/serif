// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.
// this feature is intended for NP chunk based relation models

#ifndef ENTITY_PLUS_MENTION_TYPES_RELAXED_FT_H
#define ENTITY_PLUS_MENTION_TYPES_RELAXED_FT_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "relations/discmodel/P1RelationFeatureType.h"
#include "discTagger/DTQuintgramFeature.h"
#include "discTagger/DTState.h"
#include "relations/discmodel/RelationObservation.h"
#include "relations/discmodel/RelationPropLink.h"
#include "theories/MentionSet.h"


class EntityPlusMentionTypesRelaxedFT : public P1RelationFeatureType {
public:
	EntityPlusMentionTypesRelaxedFT() : P1RelationFeatureType(Symbol(L"entity-plus-mention-types-relaxed")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		// we only fire this feature if there is a proplink

		/* will experiment with other restrictions later 
		if (o->getPropLink()->isEmpty())
			return 0;
	    */

		Symbol mtype1 = Symbol(L"CONFUSED");
		if (o->getMention1()->getMentionType() == Mention::NAME)
			mtype1 = Symbol(L"NAME");
		else if (o->getMention1()->getMentionType() == Mention::DESC)
			mtype1 = Symbol(L"DESC");
		else if (o->getMention1()->getMentionType() == Mention::PRON)
			mtype1 = Symbol(L"PRON");

		Symbol mtype2 = Symbol(L"CONFUSED");
		if (o->getMention2()->getMentionType() == Mention::NAME)
			mtype2 = Symbol(L"NAME");
		else if (o->getMention2()->getMentionType() == Mention::DESC)
			mtype2 = Symbol(L"DESC");
		else if (o->getMention2()->getMentionType() == Mention::PRON)
			mtype2 = Symbol(L"PRON");
		
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getEntityType().getName(), mtype2);
		return 1;
	}

};

#endif
