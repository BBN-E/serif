// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_LAST2LETTERS_H
#define D_T_LAST2LETTERS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DT2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFLast2LettersFeatureType : public PIdFFeatureType {
public:
	IdFLast2LettersFeatureType() : PIdFFeatureType(Symbol(L"idf-last-2-letters"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT2IntFeature(this, SymbolConstants::nullSymbol,
								  0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		Symbol word = o->getSymbol();
		const wchar_t* wordstr = word.to_string();
		size_t sz = wcslen(wordstr);
		if(sz<3){
			return 0;
		}
		resultArray[0] = _new DT2IntFeature(this, state.getTag(),
			wordstr[sz-2], wordstr[sz-1]);
		return 1;
		
	}
};

#endif
