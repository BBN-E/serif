// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MOD_CLASH_AND_MATCH_FT_H
#define MOD_CLASH_AND_MATCH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class ModClashAndModMatchFT : public DTCorefFeatureType {
public:
	ModClashAndModMatchFT() : DTCorefFeatureType(Symbol(L"mod-clash-and-mod-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool clash = DescLinkFeatureFunctions::hasModClash(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(!clash)
			return 0;
		bool match = DescLinkFeatureFunctions::hasModMatch(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(clash && match){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;

	}

};
#endif
