// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CLOSEST_MENTION_DISTANCE_FT_4_H
#define CLOSEST_MENTION_DISTANCE_FT_4_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

/*** this feature is deprecated and remains for ace eval 2007 compatibility.
* use ClosestMentDistanceFT ("ment-distance8") for better performance.
***/
class ClosestMentDistance4FT : public DTCorefFeatureType {
public:
	ClosestMentDistance4FT() : DTCorefFeatureType(Symbol(L"ment-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol mentDistance = DescLinkFeatureFunctions::getMentionDistance4(o->getMention(), 
			o->getEntity(), o->getEntitySet());
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), mentDistance);
		return 1;
	}

};
#endif
