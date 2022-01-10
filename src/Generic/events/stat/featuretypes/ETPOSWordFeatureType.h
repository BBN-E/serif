// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_POS_WORD_FEATURE_TYPE_H
#define ET_POS_WORD_FEATURE_TYPE_H

#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/parse/LanguageSpecificFunctions.h"


class ETPOSWordFeatureType : public EventTriggerFeatureType {
public:
	ETPOSWordFeatureType() : EventTriggerFeatureType(Symbol(L"pos-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		if (LanguageSpecificFunctions::isVerbPOSLabel(o->getPOS())) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"VERB"),
												o->getStemmedWord());
			return 1;
		}
		else if (LanguageSpecificFunctions::isNPtypePOStag(o->getPOS())) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"NOUN"),
												o->getStemmedWord());
			return 1;
		}
		else if (LanguageSpecificFunctions::isPrepPOSLabel(o->getPOS())) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"PREP"),
												o->getStemmedWord());
			return 1;
		}
		else if (LanguageSpecificFunctions::isAdjective(o->getPOS())) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), Symbol(L"ADJ"),
												o->getStemmedWord());
			return 1;
		}
		return 0;
	}
};

#endif
