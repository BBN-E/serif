// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PART_OF_SPEECH_N1W_WORD_FEATURE_TYPE_H
#define D_T_PART_OF_SPEECH_N1W_WORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class PartOfSpeechN1W_WordFeatureType : public PPartOfSpeechFeatureType {
public:
	PartOfSpeechN1W_WordFeatureType() : PPartOfSpeechFeatureType(Symbol(L"word,word+1"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		if(state.getIndex()+1 >= state.getNObservations()){
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o->getSymbol(), blankToken.getSymbol());
			return 1;
		}
		TokenObservation *o_1= static_cast<TokenObservation*>(
			state.getObservation(state.getIndex() +1));
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o->getSymbol(), o_1->getSymbol());
		return 1;
	}


};

#endif
