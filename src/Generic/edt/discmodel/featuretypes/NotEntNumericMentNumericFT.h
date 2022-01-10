// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NOTENTNUMERIC_AND_MENTNUMERIC_FT_H
#define NOTENTNUMERIC_AND_MENTNUMERIC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class NotEntNumericAndMentNumericFT : public DTCorefFeatureType {
public:
	  NotEntNumericAndMentNumericFT() : DTCorefFeatureType(Symbol(L"not-ent-numeric-and-ment-numeric")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool  mentnumeric  = DescLinkFeatureFunctions::mentHasNumeric( o->getMention());
		if(!mentnumeric)
			return 0;
		bool ent_has_numeric = DescLinkFeatureFunctions::entHasNumeric(o->getEntity(), 
			o->getEntitySet());
		if(ent_has_numeric)
			return 0;

		if(mentnumeric && !ent_has_numeric ){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
