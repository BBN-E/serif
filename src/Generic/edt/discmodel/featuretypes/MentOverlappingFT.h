// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_OVERLAPPING_FT_H
#define MENT_OVERLAPPING_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class MentOverlappingFT : public DTCorefFeatureType {
public:
	MentOverlappingFT() : DTCorefFeatureType(Symbol(L"ment-overlapping-ment")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		
		bool  val  = DescLinkFeatureFunctions::mentOverlapInAnotherMention( o->getMention(),o->getMentionSet());
		if(val){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
