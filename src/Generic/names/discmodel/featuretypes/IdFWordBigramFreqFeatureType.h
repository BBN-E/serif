// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

/**
* this featureType relies on a bigram frequency table.
* the feature fires for each Type according to whether the
* frequency of the bigram of words in position (i,i+1).
*/
#ifndef D_T_BIGRAM_WORD_FEATURE_TYPE_H
#define D_T_BIGRAM_WORD_FEATURE_TYPE_H

#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/names/discmodel/PIdFFeatureTypes.h"
#include "Generic/names/discmodel/FeatureTypesSymbols.h"
#include "Generic/common/ParamReader.h"

#include <iostream>

using namespace std;

class IdFWordBigramFreqFeatureType  : public PIdFFeatureType {
private:

public:
	Symbol* _bigram;

	IdFWordBigramFreqFeatureType() : PIdFFeatureType(Symbol(L"word-bigram-freq"), InfoSource::OBSERVATION)
	{
		_bigram = _new Symbol[2];
		
	}

	// check the bigram parameters exists
	virtual void validateRequiredParameters(){
		ParamReader::getRequiredParam("pidf_bigram_vocab_file");
	}

	~IdFWordBigramFreqFeatureType(void){}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
		DTFeature **resultArray) const {

		TokenObservation *o_1, *o_2;
		if((state.getIndex()-1) < 0){
			_bigram[0] = FeatureTypesSymbols::outOfBoundSym;
		}else {
			o_1 = static_cast<TokenObservation*>(
				state.getObservation(state.getIndex()-1));
			_bigram[0] = o_1->getSymbol();
		}

		o_2= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		_bigram[1] = o_2->getSymbol();

		Symbol tag = state.getTag();
		const wchar_t* txt = tag.to_string();
		const wchar_t* txt2 = _bigram[0].to_string();
		//cerr<<_bigram[0].to_string()<<"  "<<_bigram[1].to_string()<<endl;

		//cerr << "o_2=" << o_2 << endl;
		//if ( o_2 != 0 ) cerr << "table_ptr=" << o_2->getBigramCoundTable() << endl;

		int bin = static_cast<int>(o_2->getBigramCountTable()->lookup(_bigram));
		const Symbol* binSym;
		if(bin==0) binSym = &FeatureTypesSymbols::countEq0;
		else if(bin<=3) binSym = &FeatureTypesSymbols::countEq1To3;
		else if(bin<=8) binSym = &FeatureTypesSymbols::countEq4To8;
		else binSym = &FeatureTypesSymbols::countEq9OrMore;

		resultArray[0] = _new DTBigramFeature(this, tag, *binSym);
		return 1;
	}

};
#endif
