// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_FOLLOWED_BY_IF_H
#define EM_FOLLOWED_BY_IF_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

class isFollowedByIFWord : public EventModalityFeatureType {

public:
	isFollowedByIFWord() : EventModalityFeatureType(Symbol(L"is-followed-by-if-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		// option 1
		if (o->isFollowedbyIFWord()){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"TRUE"));
			return 1;
		}else{
			return 0;
		}

		// option 2
		/*
		if (o->isFollowedbyIFWord()){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"TRUE"));
		}else{
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"FALSE"));
		}
		return 1;
		*/
	}

};
#endif
