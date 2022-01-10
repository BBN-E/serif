// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PL_TYPE_MENT_HW_DISTANCE_FT_H
#define PL_TYPE_MENT_HW_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/Mention.h"


class PLTypeMentHWFT : public DTCorefFeatureType {
public:
	PLTypeMentHWFT() : DTCorefFeatureType(Symbol(L"type-ment-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
										   SymbolConstants::nullSymbol,
										   SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol type = o->getEntity()->getType().getName();
		Symbol headWord = o->getMention()->getNode()->getHeadWord();

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), type, headWord);
		return 1;
		
	}

};
#endif
