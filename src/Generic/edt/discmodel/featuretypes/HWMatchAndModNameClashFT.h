// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_MATCH_AND_MOD_NAME_CLASH_FT_H
#define HW_MATCH_AND_MOD_NAME_CLASH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HWMatchAndModNameClashFT : public DTCorefFeatureType {
public:
	HWMatchAndModNameClashFT() : DTCorefFeatureType(Symbol(L"hw-match-and-mod-name-clash")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool  val  = DescLinkFeatureFunctions::testHeadWordMatch( o->getMention(),
			o->getEntity(), o->getEntitySet());
		if(!val)
			return 0;
		bool val3 = DescLinkFeatureFunctions::hasModName(o->getMention());
		if(!val3)
			return 0;
		bool val2 = DescLinkFeatureFunctions::hasModNameEntityClash( o->getMention(),
			o->getEntity(), o->getEntitySet());

		if(val && (val3 && val2)){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
