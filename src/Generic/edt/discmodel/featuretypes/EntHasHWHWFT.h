// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENT_HAS_HW_HW_FT_H
#define ENT_HAS_HW_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"


class EntHasHWHWFT : public DTCorefFeatureType {
public:
	EntHasHWHWFT() : DTCorefFeatureType(Symbol(L"ent-has-hw+hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
										   SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol headWord = o->getMention()->getNode()->getHeadWord();
		if(DescLinkFeatureFunctions::checkEntityHasHW(o->getEntity(), o->getEntitySet(), headWord)){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), headWord);
			return 1;
		}
		return 0;
		
	}

};
#endif
