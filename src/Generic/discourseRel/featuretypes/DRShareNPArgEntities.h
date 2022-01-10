// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_shareNPArgEntities_H
#define DR_shareNPArgEntities_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "discourseRel/DiscourseRelFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "discTagger/DTState.h"
#include "discourseRel/DiscourseRelObservation.h"


class DRShareNPArgEntities : public DiscourseRelFeatureType {

public:
	DRShareNPArgEntities() : DiscourseRelFeatureType(Symbol(L"share_npArg_entities")) {
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

		bool shareNPArgEntities = o->shareNPargMentions();

		if (shareNPArgEntities){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"shareNPArgEntities"));
			return 1;
		}else{
			return 0;
		}
	}
};
#endif
