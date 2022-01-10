// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWNODE_MATCH_AND_NUMERIC_CLASH_FT_H
#define HWNODE_MATCH_AND_NUMERIC_CLASH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HWNodeMatchAndNumericClashFT : public DTCorefFeatureType {
public:
	HWNodeMatchAndNumericClashFT() : DTCorefFeatureType(Symbol(L"hw-node-match-and-numeric-clash")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
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
		bool ment_has_numeric = DescLinkFeatureFunctions::mentHasNumeric(o->getMention());
		if(!ment_has_numeric)
			return 0;
		bool ent_has_numeric = DescLinkFeatureFunctions::entHasNumeric(o->getEntity(), 
			o->getEntitySet());
		if(!ent_has_numeric)
			return 0;
		bool has_numeric_clash = DescLinkFeatureFunctions::hasNumericClash( o->getMention(),
			o->getEntity(), o->getEntitySet());

		if(val && ment_has_numeric && ent_has_numeric && has_numeric_clash ){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
