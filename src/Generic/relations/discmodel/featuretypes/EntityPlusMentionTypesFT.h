// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENTITY_PLUS_MENTION_TYPES_FT_H
#define ENTITY_PLUS_MENTION_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class EntityPlusMentionTypesFT : public P1RelationFeatureType {
private:
	Symbol _confusedSymbol;
	Symbol _nameSymbol;
	Symbol _descSymbol;
	Symbol _pronSymbol;
public:
	EntityPlusMentionTypesFT() : P1RelationFeatureType(Symbol(L"entity-plus-mention-types")),
	    _confusedSymbol(L"CONFUSED"), _nameSymbol(L"NAME"),
	    _descSymbol(L"DESC"),_pronSymbol(L"PRON") {}

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

		if (o->getPropLink()->isEmpty())
			return 0;

		Symbol mtype1 = _confusedSymbol;
		if (o->getMention1()->getMentionType() == Mention::NAME)
			mtype1 = _nameSymbol;
		else if (o->getMention1()->getMentionType() == Mention::DESC)
			mtype1 = _descSymbol;
		else if (o->getMention1()->getMentionType() == Mention::PRON)
			mtype1 = _pronSymbol;

		Symbol mtype2 = _confusedSymbol;
		if (o->getMention2()->getMentionType() == Mention::NAME)
			mtype2 = _nameSymbol;
		else if (o->getMention2()->getMentionType() == Mention::DESC)
			mtype2 = _descSymbol;
		else if (o->getMention2()->getMentionType() == Mention::PRON)
			mtype2 = _pronSymbol;
		
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getEntityType().getName(), mtype2);
		return 1;
	}

};

#endif
