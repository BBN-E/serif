// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_MENTION_CONNECTING_STRING_FEATURE_TYPE_H
#define TA_MENTION_CONNECTING_STRING_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTBigramStringFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TAMentionConnectingStringFeatureType : public RelationTimexArgFeatureType {
public:
	TAMentionConnectingStringFeatureType() :RelationTimexArgFeatureType(Symbol(L"ment-connecting-string")) {}

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

		int count = 0;

		if (o->getMention1ConnectingString() != L"")
			resultArray[count++] = _new DTBigramStringFeature(this, state.getTag(), 
				o->getRelationType(), o->getMention1ConnectingString());

		if (o->getMention2ConnectingString() != L"")
			resultArray[count++] = _new DTBigramStringFeature(this, state.getTag(), 
				o->getRelationType(), o->getMention2ConnectingString());

		return count;
	}
};

#endif
