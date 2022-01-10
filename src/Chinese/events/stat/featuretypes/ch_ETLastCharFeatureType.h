// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_ET_LAST_CHAR_FEATURE_TYPE_H
#define CH_ET_LAST_CHAR_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class ChineseETLastCharFeatureType : public EventTriggerFeatureType {
public:
	ChineseETLastCharFeatureType() : EventTriggerFeatureType(Symbol(L"last-char")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		Symbol word = o->getWord();
		const wchar_t* str = word.to_string();
		int len = static_cast<int>(wcslen(str));
		Symbol last = Symbol(&(str[len-1]));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), last);
		return 1;
	}
};

#endif
