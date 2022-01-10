// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_WORDWITHOUTAL_H
#define D_T_WORDWITHOUTAL_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include "Generic/common/SymbolUtilities.h"


class IdFWordWithoutAl : public PIdFFeatureType {
public:
	IdFWordWithoutAl() : PIdFFeatureType(Symbol(L"idf-word-without-al"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
				SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		Symbol wordwoAl = SymbolUtilities::getWordWithoutAl(word);

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), wordwoAl);
		return 1;
		
	}
};

#endif
