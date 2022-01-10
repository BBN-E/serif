// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_NUMBER_CLASH_FT_H
#define en_NUMBER_CLASH_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/Guesser.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EnglishNumberClashFT : public DTCorefFeatureType {
	Symbol CLASH_SYM;
	Symbol MATCH_SYM;
public:
	EnglishNumberClashFT() : DTCorefFeatureType(Symbol(L"number-clash")) {
		CLASH_SYM = Symbol(L"CLASH");
		MATCH_SYM = Symbol(L"MATCH");
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		// only people
		if (!o->getEntity()->getType().matchesPER())
			return 0;

		Symbol entNumber = Guesser::guessNumber(o->getEntitySet(), o->getEntity());
		Symbol mentNumber = Guesser::guessNumber(o->getMention()->getNode(), o->getMention());
		
		// only when we know something!
//		if (entGender == Guesser::UNKNOWN || mentGender == Guesser::UNKNOWN)
//			return 0;

		if (entNumber == mentNumber)	
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), MATCH_SYM);
		else
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), CLASH_SYM);

		return 1;
	}

};
#endif
