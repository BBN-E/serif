// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_leadingConn_H
#define DR_leadingConn_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "discourseRel/DiscourseRelFeatureType.h"
#include "discTagger/DTBigramFeature.h"
#include "discTagger/DTState.h"
#include "discourseRel/DiscourseRelObservation.h"


class DRLeadingConn : public DiscourseRelFeatureType {

public:
	DRLeadingConn() : DiscourseRelFeatureType(Symbol(L"leading_connective")) {
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

		bool isLedbyConn = o->sent2IsLedbyConn();

		if (isLedbyConn){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"ledbyConn"));
			resultArray[1] = _new DTBigramFeature(this, state.getTag(), o->getConnLeadingSent2());
			return 2;
		}else{
			return 0;
		}
	}
};
#endif
