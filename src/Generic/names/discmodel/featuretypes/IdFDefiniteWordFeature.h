// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_DEFINITEWORDFEATURE_H
#define D_T_DEFINITEWORDFEATURE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


#include "Generic/common/SymbolUtilities.h"


class IdFDefinteWordFeature : public PIdFFeatureType {
public:
	IdFDefinteWordFeature() : PIdFFeatureType(Symbol(L"idf-definite-word-feature"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		Symbol wordwoAl = SymbolUtilities::getWordWithoutAl(word);
		if(word == wordwoAl)
			return 0;
		else{
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		}
	}
};

#endif
