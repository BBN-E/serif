// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_FIRSTLETTER_H
#define D_T_FIRSTLETTER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFFirstLetterFeatureType : public PIdFFeatureType {
public:
	IdFFirstLetterFeatureType() : PIdFFeatureType(Symbol(L"idf-first-letter"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol,
								  0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		const wchar_t* wordstr = word.to_string();
		size_t sz = wcslen(wordstr);
		if(sz<2){
			return 0;
		}
		resultArray[0] = _new DTIntFeature(this, state.getTag(),  wordstr[0]);
		return 1;
		
	}
};

#endif
