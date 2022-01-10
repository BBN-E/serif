// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_SENTENCE_LOCATION_FEATURE_TYPE_H
#define TA_SENTENCE_LOCATION_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TASentenceLocationFeatureType : public RelationTimexArgFeatureType {
public:
	TASentenceLocationFeatureType() : RelationTimexArgFeatureType(Symbol(L"sent-location")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));


		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
			o->getRelationType(), o->getSentenceLocation());
		return 1;
	}
};

#endif
