// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_IS_PREMOD_OF_MENT_WITH_ENTTYPE_H
#define EM_IS_PREMOD_OF_MENT_WITH_ENTTYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

class isPremodOfMentionWithEntType : public EventModalityFeatureType {

public:
	isPremodOfMentionWithEntType() : EventModalityFeatureType(Symbol(L"is-premod-of-mention-with-entType")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));
		
		if (o->isPremodOfMention()){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getEntityType());
			return 1;
		}else{
			return 0;
		}

	}

};
#endif
