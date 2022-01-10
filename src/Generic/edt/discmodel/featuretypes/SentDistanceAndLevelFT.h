// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SENT_DISTANCE_ANDLEVEL_FT_H
#define SENT_DISTANCE_ANDLEVEL_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"
#include "Generic/theories/Entity.h"


class SentDistanceAndLevelFT : public DTCorefFeatureType {
public:
	SentDistanceAndLevelFT() : DTCorefFeatureType(Symbol(L"sent-distance-and-level")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		Symbol entityMentionLevel = o->getEntityMentionLevel();

		Symbol distance = DescLinkFeatureFunctions::getSentenceDistance(o->getMention()->getSentenceNumber(), 
			o->getEntity(), o->getEntitySet());
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), entityMentionLevel, distance);
		return 1;
	}

};
#endif
