// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_LEFT_ENTITY_AND_MENTION_TYPES_FT_H
#define AR_LEFT_ENTITY_AND_MENTION_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class ArabicLeftEntityAndMentionTypesFT : public P1RelationFeatureType {
public:
	ArabicLeftEntityAndMentionTypesFT() : P1RelationFeatureType(Symbol(L"left-entity-and-mention-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		Symbol mtype1 = Symbol(L"CONFUSED");
		if (o->getMention1()->getMentionType() == Mention::NAME)
			mtype1 = Symbol(L"NAME");
		else if (o->getMention1()->getMentionType() == Mention::DESC)
			mtype1 = Symbol(L"DESC");
		else if (o->getMention1()->getMentionType() == Mention::PRON)
			mtype1 = Symbol(L"PRON");

		
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1);
		return 1;
	}

};

#endif
