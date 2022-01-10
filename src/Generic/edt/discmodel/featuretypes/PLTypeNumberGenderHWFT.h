// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PL_TYPE_NUMBER_GENDER_HW_FT_H
#define PL_TYPE_NUMBER_GENDER_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/Guesser.h"

#include "Generic/theories/Mention.h"


class PLTypeNumberGenderHWFT : public DTCorefFeatureType {
public:
	PLTypeNumberGenderHWFT() : DTCorefFeatureType(Symbol(L"type-number-gender-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, 
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		Symbol type = o->getEntity()->getType().getName();
		Symbol headWord = o->getMention()->getNode()->getHeadWord();

		Mention *lastEntMent = o->getLastEntityMention();

		if (lastEntMent != 0) {
			Symbol number = Guesser::guessNumber(lastEntMent->getNode(), lastEntMent);
			Symbol gender = Guesser::guessGender(lastEntMent->getNode(), lastEntMent);

			resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), type,
													 number, gender, headWord);
			return 1;
		}
		return 0;
	}

};
#endif
