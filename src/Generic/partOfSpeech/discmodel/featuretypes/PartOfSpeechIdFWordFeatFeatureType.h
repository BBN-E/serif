// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PART_OF_SPEECH_PartOfSpeech_WORD_FEATURE_TYPE_H
#define D_T_PART_OF_SPEECH_PartOfSpeech_WORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class PartOfSpeechIdFWordFeatFeatureType : public PPartOfSpeechFeatureType {
public:
	PartOfSpeechIdFWordFeatFeatureType() : PPartOfSpeechFeatureType(Symbol(L"idfwordfeat"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getIdFWordFeature());
		return 1;
	}

};

#endif
