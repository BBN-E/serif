// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PART_OF_SPEECH_PREV_WC20_FEATURE_TYPE_H
#define D_T_PART_OF_SPEECH_PREV_WC20_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class PartOfSpeechPrevWC20FeatureType : public PPartOfSpeechFeatureType {
public:
	PartOfSpeechPrevWC20FeatureType() : PPartOfSpeechFeatureType(Symbol(L"prev-wc20"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() <= 0){
			resultArray[0] = _new DTIntFeature(this, state.getTag(), blankWordClass.c20());
			return 1;
		}
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()-1));

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c20());
		return 1;
	}

};
#endif
