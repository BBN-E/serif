// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_NEXTBIGRAM_WITH_CUTT_OFF_FEATURE_TYPE_H
#define D_T_IDF_NEXTBIGRAM_WITH_CUTT_OFF_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/names/discmodel/FeatureTypesSymbols.h"
#include "Generic/common/ParamReader.h"


class IdFNextBigramCutOffFeatureType : public PIdFFeatureType {
public:
	Symbol* _bigram;

	IdFNextBigramCutOffFeatureType() : PIdFFeatureType(Symbol(L"nextbigram_with_cuttoff"), InfoSource::OBSERVATION) {
		_bigram = _new Symbol[2];
	}

	// check the bigram parameters exists
	virtual void validateRequiredParameters(){
		cerr<<"checking params for IdFNextBigramCutOffFeatureType"<<endl;
		ParamReader::getRequiredParam("pidf_bigram_vocab_file");
	}

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
		TokenObservation *o_1,*o_2;

		o_1 = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		_bigram[0] = o_1->getSymbol();
		if((state.getIndex()+1) >= state.getNObservations()){
			_bigram[0] = FeatureTypesSymbols::outOfBoundSym;
		}else {
			o_2= static_cast<TokenObservation*>(
				state.getObservation(state.getIndex()+1));
			_bigram[1] = o_2->getSymbol();
		}


		int bin = static_cast<int>(o_2->getBigramCountTable()->lookup(_bigram));
		if(bin<2) return 0;
		//else
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o_1->getSymbol(),
			o_2->getSymbol());
		return 1;
	}




};

#endif
