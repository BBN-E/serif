// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MOD_CLASH_AND_UMRATIO_FT_H
#define MOD_CLASH_AND_UMRATIO_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

//since this feature is only defined if a mod clash exists, no need to include 
//existence in the feature
class ModClashAndUMRatioFT : public DTCorefFeatureType {
public:
	ModClashAndUMRatioFT() : DTCorefFeatureType(Symbol(L"mod-clash-and-um-ratio")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool hasmod = DescLinkFeatureFunctions::hasModClash(o->getMention(),
			o->getEntity(),o->getEntitySet());
		if(!hasmod)
			return 0;
		Symbol umratio = DescLinkFeatureFunctions::getUniqueModifierRatio(o->getMention(),
			o->getEntity(),o->getEntitySet());
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), umratio);
			return 1;

	}

};
#endif
