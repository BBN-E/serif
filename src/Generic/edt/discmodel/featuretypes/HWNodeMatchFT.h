// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_NODE_MATCH_FT_H
#define HW_NODE_MATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HWNodeMatchFT : public DTCorefFeatureType {
public:
	HWNodeMatchFT() : DTCorefFeatureType(Symbol(L"hw-node-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool match = DescLinkFeatureFunctions::testHeadWordMatch(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(!match)
			return 0;
		bool nodematch =DescLinkFeatureFunctions::testHeadWordNodeMatch(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(nodematch){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
