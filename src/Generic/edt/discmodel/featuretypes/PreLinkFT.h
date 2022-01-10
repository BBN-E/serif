// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRE_LINK_FT_H
#define PRE_LINK_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class PreLinkFT : public DTCorefFeatureType {
public:
	PreLinkFT() : DTCorefFeatureType(Symbol(L"pre-link")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		Symbol preLink = o->getPreLinkSymbol();
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), preLink);
		return 1;
	}

};
#endif
