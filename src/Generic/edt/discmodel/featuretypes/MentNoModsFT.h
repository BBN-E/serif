// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENT_NO_MODS_FT_H
#define MENT_NO_MODS_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class MentNoModsFT : public DTCorefFeatureType {
public:
	MentNoModsFT() : DTCorefFeatureType(Symbol(L"ment-no-mods")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		std::set<Symbol> results = DescLinkFeatureFunctions::getMentMods(o->getMention());
		if (results.empty()) {
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
