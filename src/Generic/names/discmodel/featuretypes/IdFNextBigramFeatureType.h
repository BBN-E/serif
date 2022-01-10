// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_NEXTBIGRAM_FEATURE_TYPE_H
#define D_T_IDF_NEXTBIGRAM_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFNextBigramFeatureType : public PIdFFeatureType {
public:
	IdFNextBigramFeatureType() : PIdFFeatureType(Symbol(L"nextbigram"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if((state.getIndex()+1) >= state.getNObservations()){
			return 0;
		}
		TokenObservation *o_1 = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		TokenObservation *o_2= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()+1));

		if (PIdFFeatureType::isVocabBigram(o_1->getSymbol(), o_2->getSymbol())) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
				o_1->getSymbol(), o_2->getSymbol());
			return 1;
		} else {
			return 0;
		}
	}


};

#endif
