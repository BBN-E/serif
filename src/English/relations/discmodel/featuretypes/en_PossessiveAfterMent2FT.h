// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_POSSESSIVE_AFTER_MENT2_FT_H
#define EN_POSSESSIVE_AFTER_MENT2_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class EnglishPossessiveAfterMent2FT : public P1RelationFeatureType {
public:
	EnglishPossessiveAfterMent2FT() : P1RelationFeatureType(Symbol(L"poss-after-ment2")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if (o->hasPossessiveAfterMent2()) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(),
								  Symbol(L"TRUE"));
	
			return 1;
		}

		
		return 0;
	}

};

#endif
