// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DISTANCE_FT_H
#define DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


/** this feature is deprecated. The same feature exists under the name "sent-distance"
 * this feature is left for backward compatibility
 */
class DistanceFT : public DTCorefFeatureType {
public:
	DistanceFT() : DTCorefFeatureType(Symbol(L"distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol distance = DescLinkFeatureFunctions::getSentenceDistance(o->getMention()->getSentenceNumber(), 
			o->getEntity(), o->getEntitySet());
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), distance);
		return 1;
	}

};
#endif
