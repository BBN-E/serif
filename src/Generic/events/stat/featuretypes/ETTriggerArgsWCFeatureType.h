// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_TRIGGER_ARGS_WC_FEATURE_TYPE_H
#define ET_TRIGGER_ARGS_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"


class ETTriggerArgsWCFeatureType : public EventTriggerFeatureType {
public:
	ETTriggerArgsWCFeatureType() : EventTriggerFeatureType(Symbol(L"trigger-args-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		int i = 0;
		WordClusterClass wc(o->getWord());
		for ( ; i < EventTriggerObservation::_MAX_OTHER_ARGS; i++) {
			Symbol sym = o->getOtherArgToTrigger(i);
			if (sym.is_null())
				return i;
			resultArray[i] = _new DTBigramIntFeature(this, state.getTag(),
				sym, wc.c16());
		}

		return 1;
	}
};

#endif
