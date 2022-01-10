// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_TRIGGER_ARGS_FEATURE_TYPE_H
#define ET_TRIGGER_ARGS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETTriggerArgsFeatureType : public EventTriggerFeatureType {
public:
	ETTriggerArgsFeatureType() : EventTriggerFeatureType(Symbol(L"trigger-args")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		int i = 0;
		for ( ; i < EventTriggerObservation::_MAX_OTHER_ARGS; i++) {
			Symbol sym = o->getOtherArgToTrigger(i);
			if (sym.is_null())
				return i;
			resultArray[i] = _new DTTrigramFeature(this, state.getTag(),
				o->getStemmedWord(), sym);
		}

		return 1;
	}
};

#endif
