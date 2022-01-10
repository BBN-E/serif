// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTOVERLAPPING_FT_H
#define MENTOVERLAPPING_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

/** mention overlapping the entity */
class OverlappingFT : public DTCorefFeatureType {
public:
	OverlappingFT() : DTCorefFeatureType(Symbol(L"overlapping")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		bool  val  = DescLinkFeatureFunctions::mentOverlapsEntity( o->getMention(),
			o->getEntity(), o->getEntitySet());
		if(val){
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
