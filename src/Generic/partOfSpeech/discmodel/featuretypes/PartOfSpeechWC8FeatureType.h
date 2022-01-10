// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PART_OF_SPEECH_WC8_FEATURE_TYPE_H
#define D_T_PART_OF_SPEECH_WC8_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class PartOfSpeechWC8FeatureType : public PPartOfSpeechFeatureType {
public:
	PartOfSpeechWC8FeatureType() : PPartOfSpeechFeatureType(Symbol(L"wc8"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c8());
		return 1;
	}

};

#endif
