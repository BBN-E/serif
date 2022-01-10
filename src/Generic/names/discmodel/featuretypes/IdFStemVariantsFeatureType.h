// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_STEMVARIANTS_FEATURETYPE_H
#define D_T_STEMVARIANTS_FEATURETYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include "Generic/common/SymbolUtilities.h"


class IdFStemVariantsFeatureType : public PIdFFeatureType {
public:
	IdFStemVariantsFeatureType() : PIdFFeatureType(Symbol(L"idf-stem-variants"), InfoSource::OBSERVATION) {}

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
		Symbol variants[15];
		int nvariants = SymbolUtilities::getStemVariants(word, variants, 15);
		bool wordisvar = false;
		int i = 0;
		for(i=0; i< nvariants; i++){
			if(variants[i] == word)
				wordisvar = true;
            resultArray[i] = _new DTBigramFeature(this, state.getTag(), variants[i]);
		}
		if(!wordisvar){
			resultArray[i++] = _new DTBigramFeature(this, state.getTag(), word);
		}
		return i;
	}
};

#endif
