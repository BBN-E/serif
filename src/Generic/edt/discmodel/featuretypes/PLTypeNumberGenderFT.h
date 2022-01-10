// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PL_TYPE_NUMBER_GENDER_FT_H
#define PL_TYPE_NUMBER_GENDER_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/Guesser.h"

#include "Generic/theories/Mention.h"


class PLTypeNumberGenderFT : public DTCorefFeatureType {
public:
	PLTypeNumberGenderFT() : DTCorefFeatureType(Symbol(L"type-number-gender")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
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

		Mention *lastEntMent = o->getLastEntityMention();

		if (lastEntMent != 0) {
			Symbol number = Guesser::guessNumber(lastEntMent->getNode(), lastEntMent);
			Symbol gender = Guesser::guessGender(lastEntMent->getNode(), lastEntMent);

			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), type,
													number, gender);
			return 1;
		}
		return 0;
	}

};
#endif
