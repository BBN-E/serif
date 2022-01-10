// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MENTOVERLAPPINGHW_FT_H
#define MENTOVERLAPPINGHW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

/** mention overlapping the entity */
class OverlappingHWFT : public DTCorefFeatureType {
public:
	OverlappingHWFT() : DTCorefFeatureType(Symbol(L"overlapping-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, 
									SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		bool val = DescLinkFeatureFunctions::mentOverlapsEntity(o->getMention(),
			o->getEntity(), o->getEntitySet());
		if (val) {
			Symbol headWord = o->getMention()->getNode()->getHeadWord();
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), headWord);
			return 1;
		}
		else
			return 0;
	}

};
#endif
