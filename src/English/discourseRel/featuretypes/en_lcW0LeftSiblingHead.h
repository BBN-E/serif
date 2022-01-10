// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_lcW0_LeftSiblingHead_H
#define en_lcW0_LeftSiblingHead_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"


class EnglishLcW0LeftSiblingHead : public DiscourseRelFeatureType {

public:
	EnglishLcW0LeftSiblingHead() : DiscourseRelFeatureType(Symbol(L"word_leftSibling_head")) {
	
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o->getLCWord(), o->getLeftSiblingHead());

		return 1;
	}

};
#endif
