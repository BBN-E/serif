// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_share2NPArgEntities_H
#define DR_share2NPArgEntities_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "discourseRel/DiscourseRelFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "discTagger/DTState.h"
#include "discourseRel/DiscourseRelObservation.h"


class DRShare2NPArgEntities : public DiscourseRelFeatureType {

public:
	DRShare2NPArgEntities() : DiscourseRelFeatureType(Symbol(L"share_2_npArg_entities")) {
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

		bool share2NPArgEntities = o->share2NPargMentions();

		if (share2NPArgEntities){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"share2NPArgEntities"));
			return 1;
		}else{
			return 0;
		}
	}
};
#endif
