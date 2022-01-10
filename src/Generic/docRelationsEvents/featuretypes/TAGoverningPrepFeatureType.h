// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_GOVERNING_PREP_FEATURE_TYPE_H
#define TA_GOVERNING_PREP_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TAGoverningPrepFeatureType : public RelationTimexArgFeatureType {
public:
	TAGoverningPrepFeatureType() :RelationTimexArgFeatureType(Symbol(L"governing-prep")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		if (o->getGoverningPrep().is_null())
			return 0;

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
			o->getGoverningPrep());
		return 1;
	}
};

#endif
