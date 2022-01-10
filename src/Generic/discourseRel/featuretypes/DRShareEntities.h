// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_shareEntities_H
#define DR_shareEntities_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "discourseRel/DiscourseRelFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "discTagger/DTState.h"
#include "discourseRel/DiscourseRelObservation.h"


class DRShareEntities : public DiscourseRelFeatureType {

public:
	DRShareEntities() : DiscourseRelFeatureType(Symbol(L"share_entities")) {
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));

		bool shareEntities = o->shareMentions();

		if (shareEntities){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"shareEntities"));
			return 1;
		}else{
			return 0;
		}
	}
};
#endif
