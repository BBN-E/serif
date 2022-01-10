// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_LCWORD_FEATURE_TYPE_H
#define D_T_LCWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/partOfSpeech/discmodel/PPartOfSpeechFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class PartOfSpeechLCWordFeatureType : public PartOfSpeechFeatureType {
public:
	PartOfSpeechLCWordFeatureType() : PPartOfSpeechFeatureType(Symbol(L"lc-word"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getLCSymbol());
		return 1;
	}
};

#endif
