// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_RIGHTSIBLINGPOS_FEATURE_TYPE_H
#define DR_RIGHTSIBLINGPOS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class DRRightSiblingPOSFeatureType : public DiscourseRelFeatureType {
public:
	DRRightSiblingPOSFeatureType() : DiscourseRelFeatureType(Symbol(L"pos-rightSibling")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getRightSiblingPOS());
		return 1;
	}
};

#endif
