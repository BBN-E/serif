// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_PRIOR_FEATURE_TYPE_H
#define EL_PRIOR_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"


class ELPriorFeatureType : public EventLinkFeatureType {
public:
	ELPriorFeatureType() : EventLinkFeatureType(Symbol(L"prior")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
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
		
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
							v1->getEventType());
		return 1;
	}
};

#endif
