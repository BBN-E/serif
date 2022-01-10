// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ET_WORDNET_FT_H
#define EN_ET_WORDNET_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"

class EnglishETWordNetFeatureType : public EventTriggerFeatureType {
public:
	EnglishETWordNetFeatureType() : EventTriggerFeatureType(Symbol(L"wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));
		
		//if(o->getWord()==Symbol(L":founding")){
		//	int a=a+1;
		//}
		int offset = 1;
		int nfeatures = 0;
		while (true) {
			int value = o->getReversedNthOffset(offset);
			if (value != -1) {
				resultArray[nfeatures++] = _new DTIntFeature(this, state.getTag(), value);
			} else break;
			offset += 3;
		}

		return nfeatures;
	}

};

#endif
