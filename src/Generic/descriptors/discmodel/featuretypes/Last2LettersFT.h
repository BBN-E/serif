// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_DESC_LAST2LETTERS_H
#define D_T_DESC_LAST2LETTERS_H


#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DT2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class Last2LettersFT : public P1DescFeatureType {
public:
	Last2LettersFT() : P1DescFeatureType(Symbol(L"last-2-letters")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT2IntFeature(this, SymbolConstants::nullSymbol,
								  0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(state.getIndex()));
		Symbol word = o->getNode()->getHeadWord();
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
