// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_EL_STEMMED_TRIGGER_WORDS_FEATURE_TYPE_H
#define EN_EL_STEMMED_TRIGGER_WORDS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EventMention.h"


class EnglishELStemmedTriggerWordsFeatureType : public EventLinkFeatureType {
public:
	EnglishELStemmedTriggerWordsFeatureType() : EventLinkFeatureType(Symbol(L"stemmed-trigger-words")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
										   SymbolConstants::nullSymbol,
										   SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventLinkObservation *o = static_cast<EventLinkObservation*>(
			state.getObservation(0));

		EventMention *v1 = o->getVMention1();
		EventMention *v2 = o->getVMention2();

		Symbol v1Trigger = v1->getAnchorNode()->getHeadWord();
		Symbol v1Tag = v1->getAnchorNode()->getHeadPreterm()->getTag();
		Symbol v2Trigger = v2->getAnchorNode()->getHeadWord();
		Symbol v2Tag = v2->getAnchorNode()->getHeadPreterm()->getTag();

		Symbol stem1 = SymbolUtilities::stemWord(v1Trigger, v1Tag);
		Symbol stem2 = SymbolUtilities::stemWord(v2Trigger, v2Tag);

		if (stem1 == stem2)
			return 0;
		else if (stem1 < stem2) 
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), stem1, stem2);
		else
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), stem2, stem1);
		
		return 1;
	}
};

#endif
