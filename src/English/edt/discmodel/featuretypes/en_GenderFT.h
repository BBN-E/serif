// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_GENDER_FT_H
#define en_GENDER_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/edt/Guesser.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class EnglishGenderFT : public DTCorefFeatureType {
public:
	EnglishGenderFT() : DTCorefFeatureType(Symbol(L"gender")) {}

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

		Symbol entGender = Guesser::guessGender(o->getEntitySet(), o->getEntity());
		Symbol mentGender = Guesser::guessGender(o->getMention()->getNode(), o->getMention());
		
		// only when we know something!
		if (entGender == Guesser::UNKNOWN || mentGender == Guesser::UNKNOWN)
			return 0;

		if (entGender == mentGender)	
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L":MATCH"));
		else
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L":CLASH"));

		return 1;
	}

};
#endif
