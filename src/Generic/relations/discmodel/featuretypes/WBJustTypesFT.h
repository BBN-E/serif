// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef WORDS_BETWEEN_JUST_TYPES_FT_H
#define WORDS_BETWEEN_JUST_TYPES_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramStringFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class WBJustTypesFT : public P1RelationFeatureType {
public:
	WBJustTypesFT() : P1RelationFeatureType(Symbol(L"wb-just-entity-types")) {}

	virtual DTFeature *makeEmptyFeature() const {
		wstring dummy(L"");
		return _new DTTrigramStringFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								 dummy, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->isTooLong())
			return 0;
		
		resultArray[0] = _new DTTrigramStringFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), 
								  o->getWordsBetween(), 
								  o->getMention2()->getEntityType().getName());
		return 1;
	}

};

#endif
