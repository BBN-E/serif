// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_HW_ENT_GNT_DPCT_FT_H
#define en_HW_ENT_GNT_DPCT_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/Guesser.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"

// same as en_HWentGNT_FT (with different feature name) for backward compatability
// TODO: delete later
class EnglishEn_HWentGNT_Deprecated_FT : public DTCorefFeatureType {
public:
	EnglishEn_HWentGNT_Deprecated_FT() : DTCorefFeatureType(Symbol(L"hw-entity-gnt")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol entityType = o->getEntity()->getType().getName();
		Symbol gender = Guesser::guessGender(o->getEntitySet(), o->getEntity());
		Symbol number = Guesser::guessNumber(o->getEntitySet(), o->getEntity());
		Symbol headword = o->getMention()->getNode()->getHeadWord();
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), entityType,
			gender, number, headword);
		return 1;
	}

};
#endif
