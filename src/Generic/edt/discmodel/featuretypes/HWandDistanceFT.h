// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_AND_DISTANCE_FT_H
#define HW_AND_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"


#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"


class HWandDistanceFT : public DTCorefFeatureType {
public:
	HWandDistanceFT() : DTCorefFeatureType(Symbol(L"hw-and-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		Symbol distance = DescLinkFeatureFunctions::getSentenceDistance(o->getMention()->getSentenceNumber(), 
			o->getEntity(), o->getEntitySet());

		Symbol headSym = o->getMention()->getHead()->getHeadWord();
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), headSym, distance);
		return 1;
	}

};
#endif
