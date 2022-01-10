// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_SAME_TRIGGER_FEATURE_TYPE_H
#define EL_SAME_TRIGGER_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EventMention.h"


class ELSameTriggerFeatureType : public EventLinkFeatureType {
public:
	ELSameTriggerFeatureType() : EventLinkFeatureType(Symbol(L"same-trigger")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventLinkObservation *o = static_cast<EventLinkObservation*>(
			state.getObservation(0));

		EventMention *v1 = o->getVMention1();
		EventMention *v2 = o->getVMention2();

		int count = 0;
		if (v1->getAnchorNode()->getHeadWord() == v2->getAnchorNode()->getHeadWord()) {
			resultArray[count++] = _new DTBigramFeature(this, state.getTag(),
														v1->getEventType());
		}
		
		return count;
	}
};

#endif
