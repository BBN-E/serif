// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_wordNetPair_H
#define en_wordNetPair_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discTagger/DT2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"


class EnglishWordNetPair : public DiscourseRelFeatureType {

public:
	EnglishWordNetPair() : DiscourseRelFeatureType(Symbol(L"wordNet_pairs")) {
	
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT2IntFeature(this, SymbolConstants::nullSymbol, 
			0,0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));

		vector<int> _wordNetIds1 = o->getSynsetIdsInCurrentSent();
		vector<int> _wordNetIds2 = o->getSynsetIdsInNextSent();
		
		int maxSize = 0;
		int actualSize = (int)_wordNetIds1.size() * (int)_wordNetIds2.size();
		if ( actualSize > MAX_FEATURES_PER_EXTRACTION){
			maxSize = MAX_FEATURES_PER_EXTRACTION;
		}else{
			maxSize = actualSize;
		}
		int k = 0;
		for (int i=0; i < (int)_wordNetIds1.size(); i++){
			for (int j=0; j < (int)_wordNetIds2.size(); j++){
				resultArray[k] = _new DT2IntFeature(this, state.getTag(), _wordNetIds1[i], _wordNetIds2[j]);
				k++;
				if (k == maxSize){
					return maxSize;
				}
			}
		}
		return maxSize;
	}

};
#endif
