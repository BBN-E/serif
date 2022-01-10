// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ET_TRIGGER_ARGS_WN_FEATURE_TYPE_H
#define EN_ET_TRIGGER_ARGS_WN_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"


class EnglishETTriggerArgsWNFeatureType : public EventTriggerFeatureType {
public:
	EnglishETTriggerArgsWNFeatureType() : EventTriggerFeatureType(Symbol(L"trigger-args-wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		int offset = 1;
		int nfeatures = 0;
		while (true) {
			int value = o->getReversedNthOffset(offset);
			if (value != -1) {
				for (int i = 0 ; i < EventTriggerObservation::_MAX_OTHER_ARGS; i++) {
					Symbol sym = o->getOtherArgToTrigger(i);
					if (sym.is_null())
						break;
					resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(),
						sym, value);
					if (nfeatures == DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
						return nfeatures;
				}
			} else break;
			offset += 3;
		}

		
		return nfeatures;
	}
};

#endif
