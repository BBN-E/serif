// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_ENTITY_AND_MENTION_TYPES_FT_H
#define AR_RIGHT_ENTITY_AND_MENTION_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class ArabicRightEntityAndMentionTypesFT : public P1RelationFeatureType {
public:
	ArabicRightEntityAndMentionTypesFT() : P1RelationFeatureType(Symbol(L"right-entity-and-mention-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		Symbol mtype2 = Symbol(L"CONFUSED");
		if (o->getMention2()->getMentionType() == Mention::NAME)
			mtype2 = Symbol(L"NAME");
		else if (o->getMention2()->getMentionType() == Mention::DESC)
			mtype2 = Symbol(L"DESC");
		else if (o->getMention2()->getMentionType() == Mention::PRON)
			mtype2 = Symbol(L"PRON");

		
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
								  o->getMention2()->getEntityType().getName(), mtype2);
		return 1;
	}

};

#endif
