// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_LAST3LETTERS_H
#define D_T_LAST3LETTERS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFLast3LettersFeature : public PIdFFeatureType {
public:
	IdFLast3LettersFeature() : PIdFFeatureType(Symbol(L"idf-last-3-letters")) {}

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
		const wchar_t* wordstr = word.to_string();
		size_t sz = wcslen(wordstr);
		if(sz<4){
			return 0;
		}
		wchar_t buffer[4];
		buffer[0] = wordstr[sz-3];
		buffer[1] = wordstr[sz-2];
		buffer[2] = wordstr[sz-1];
		buffer[3] = L'\0';
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(buffer));
		return 1;
		
	}
};

#endif
