// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTNUMERIC_AND_ENTNUMERIC_ANDMATCH_FT_H
#define MENTNUMERIC_AND_ENTNUMERIC_ANDMATCH_FT_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "edt/discmodel/DTCorefFeatureType.h"
#include "discTagger/DTMonogramFeature.h"
#include "discTagger/DTState.h"
#include "edt/discmodel/DTCorefObservation.h"
#include "edt/DescLinkFeatureFunctions.h"

#include "theories/Mention.h"


class MentNumericAndEntNumericAndMatchFT : public DTCorefFeatureType {
public:
	MentNumericAndEntNumericAndMatchFT() : DTCorefFeatureType(Symbol(L"ment-numeric-and-ent-numeric-and-match")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Symbol results[5];
		bool ment_has_numeric = DescLinkFeatureFunctions::mentHasNumeric(o->getMention());
		if(!ment_has_numeric)
			return 0;
		bool ent_has_numeric = DescLinkFeatureFunctions::entHasNumeric(o->getEntity(), 
			o->getEntitySet());
		if(!ent_has_numeric)
			return 0;
		bool has_numeric_match = DescLinkFeatureFunctions::hasNumericMatch( o->getMention(),
			o->getEntity(), o->getEntitySet());


		if(ment_has_numeric && ent_has_numeric  && has_numeric_match ){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
