// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_POS_CONNECTING_STRING_FEATURE_TYPE_H
#define TA_POS_CONNECTING_STRING_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTBigramStringFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TAPOSConnectingStringFeatureType : public RelationTimexArgFeatureType {
public:
	TAPOSConnectingStringFeatureType() : RelationTimexArgFeatureType(Symbol(L"pos-connecting-string")) {}

	virtual DTFeature *makeEmptyFeature() const {
		wstring dummy(L"");
		return _new DTBigramStringFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, dummy);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		if (o->getPOSConnectingString() == L"")
			return 0;

		resultArray[0] = _new DTBigramStringFeature(this, state.getTag(), 
			o->getRelationType(), o->getPOSConnectingString());
		return 1;
	}
};

#endif
