// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_NEXTBIGRAM_WC_FEATURE_TYPE_H
#define D_T_IDF_NEXTBIGRAM_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"

class IdFNextBigramWCFeatureType : public PIdFFeatureType {
public:
	IdFNextBigramWCFeatureType() : PIdFFeatureType(Symbol(L"nextbigram-wc"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex()+1 >= state.getNObservations()){
			return 0;
		}

		int n_results = 0;

		TokenObservation *o_1 = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		TokenObservation *o_2= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()+1));
		
		wchar_t bigram[MAX_TOKEN_SIZE*2 + 1]; 
		wcsncpy(bigram, o_1->getSymbol().to_string(), MAX_TOKEN_SIZE + 1);
		wcsncat(bigram, o_2->getSymbol().to_string(), MAX_TOKEN_SIZE);
		WordClusterClass wordClass = WordClusterClass(Symbol(bigram));

		if (wordClass.c8() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c8());
		if (wordClass.c12() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c12());
		if (wordClass.c16() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c16());
		if (wordClass.c20() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c20());
		
		return n_results;
	}


};

#endif
