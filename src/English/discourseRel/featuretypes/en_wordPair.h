// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_wordPair_H
#define en_wordPair_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"


class EnglishWordPair : public DiscourseRelFeatureType {

public:
	EnglishWordPair() : DiscourseRelFeatureType(Symbol(L"word_pairs")) {
	
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));

		vector<Symbol> _bagsOfWordPairs = o->getWordPairs();
		
		int maxSize = 0;
		if ((int)_bagsOfWordPairs.size() > MAX_FEATURES_PER_EXTRACTION){
			maxSize = MAX_FEATURES_PER_EXTRACTION;
		}else{
			maxSize = (int)_bagsOfWordPairs.size();
		}

		for (int i=0; i < maxSize; i++){
			resultArray[i] = _new DTBigramFeature(this, state.getTag(), _bagsOfWordPairs[i]);
		}
		return maxSize;
	}

};
#endif
