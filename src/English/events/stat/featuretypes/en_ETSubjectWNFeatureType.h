// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ET_SUBJECT_WN_FEATURE_TYPE_H
#define EN_ET_SUBJECT_WN_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"


class EnglishETSubjectWNFeatureType : public EventTriggerFeatureType {
public:
	EnglishETSubjectWNFeatureType() : EventTriggerFeatureType(Symbol(L"subject-wordnet")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		Symbol subject = o->getSubjectOfTrigger();
		if (subject.is_null())
			return 0;

		int offset = 1;
		int nfeatures = 0;
		while (true) {
			int value = o->getReversedNthOffset(offset);
			if (value != -1) {
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, 
					state.getTag(), subject, value);
			} else break;
			offset += 3;
		}
		return nfeatures;
	}
};

#endif
