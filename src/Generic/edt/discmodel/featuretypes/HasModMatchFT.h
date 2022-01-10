// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HAS_MOD_MATCH_FT_H
#define HAS_MOD_MATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HasModMatchFT : public DTCorefFeatureType {
public:
	HasModMatchFT() : DTCorefFeatureType(Symbol(L"has-mod-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool hasmod = DescLinkFeatureFunctions::hasModMatch(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(hasmod){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
