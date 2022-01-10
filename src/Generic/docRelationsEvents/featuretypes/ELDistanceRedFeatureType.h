// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_DISTANCE_RED_FEATURE_TYPE_H
#define EL_DISTANCE_RED_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"


class ELDistanceRedFeatureType : public EventLinkFeatureType {
public:
	ELDistanceRedFeatureType() : EventLinkFeatureType(Symbol(L"distance-red")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventLinkObservation *o = static_cast<EventLinkObservation*>(
			state.getObservation(0));

		EventMention *v1 = o->getVMention1();
		EventMention *v2 = o->getVMention2();

		if (v1->getEventType() != v2->getEventType())
			return 0;
		
		int difference = v1->getSentenceNumber() - v2->getSentenceNumber();
		if (difference < 0)
			difference *= -1;

		if (difference == 0) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getReducedEventType(), Symbol(L"same-sentence"));
		} else if (difference == 1) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getReducedEventType(), Symbol(L"adjacent-sentence"));
		} else if (difference < 5) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getReducedEventType(), Symbol(L"within-5-sentences"));
		} else {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getReducedEventType(), Symbol(L"beyond-5-sentences"));
		}

		return 1;
	}
};

#endif
