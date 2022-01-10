// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWNODE_MATCH_AND_UMRATIO_FT_H
#define HWNODE_MATCH_AND_UMRATIO_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HWNodeMatchAndUMRatioFT : public DTCorefFeatureType {
public:
	HWNodeMatchAndUMRatioFT() : DTCorefFeatureType(Symbol(L"hw-node-match-and-umratio")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool  val  = DescLinkFeatureFunctions::testHeadWordNodeMatch( o->getMention(),
			o->getEntity(), o->getEntitySet());
		if(!val)
			return 0;
		Symbol umratio = DescLinkFeatureFunctions::getUniqueModifierRatio( o->getMention(),
			o->getEntity(), o->getEntitySet());
		if(val){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), umratio);
			return 1;
		}
		else
			return 0;
	}

};
#endif
