// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_MENT_POSSESIVE_FT_H
#define EN_MENT_POSSESIVE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
//#include "Generic/edt/DescLinkFeatureFunctions.h"
#include "Generic/common/WordConstants.h"
#include "Generic/theories/Mention.h"


class EnglishMentPossesiveFT : public DTCorefFeatureType {
public:
	EnglishMentPossesiveFT() : DTCorefFeatureType(Symbol(L"ment-possesive")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTNoneCorefObservation *o = static_cast<DTNoneCorefObservation*>(
			state.getObservation(0));
		
		if (WordConstants::isPossessivePronoun(o->getMention()->getHead()->getHeadWord())) {
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
		else
			return 0;
	}

};
#endif
