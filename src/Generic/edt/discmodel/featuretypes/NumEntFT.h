// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUM_ENT_FT_H
#define NUM_ENT_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

class NumEntFT : public DTCorefFeatureType {
public:
	NumEntFT() : DTCorefFeatureType(Symbol(L"num-entities")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		Symbol num_ents = DescLinkFeatureFunctions::getNumEnts(o->getEntitySet());
			
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), num_ents);
		return 1;
	}

};
#endif
