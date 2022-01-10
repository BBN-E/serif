// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_EL_SAME_TRIGGER_RED_FEATURE_TYPE_H
#define EN_EL_SAME_TRIGGER_RED_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"


class EnglishELSameTriggerRedFeatureType : public EventLinkFeatureType {
public:
	EnglishELSameTriggerRedFeatureType() : EventLinkFeatureType(Symbol(L"same-trigger-red")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventLinkObservation *o = static_cast<EventLinkObservation*>(
			state.getObservation(0));

		EventMention *v1 = o->getVMention1();
		EventMention *v2 = o->getVMention2();

		if (v1->getEventType() != v2->getEventType())
			return 0;

		// obviously if it's the SAME node, it's not coreferent
		if (v1->getAnchorNode() == v2->getAnchorNode())
			return 0;
				
		//int difference = v1->getSentenceNumber() - v2->getSentenceNumber();
		//if (difference < 0)
		//	difference *= -1;
				
		//if (difference >= 5)
		//	return 0;

		Symbol v1Trigger = v1->getAnchorNode()->getHeadWord();
		Symbol v1Tag = v1->getAnchorNode()->getHeadPreterm()->getTag();
		Symbol v2Trigger = v2->getAnchorNode()->getHeadWord();
		Symbol v2Tag = v2->getAnchorNode()->getHeadPreterm()->getTag();

		Symbol stem1 = SymbolUtilities::stemWord(v1Trigger, v1Tag);
		Symbol stem2 = SymbolUtilities::stemWord(v2Trigger, v2Tag);

		if (stem1 == stem2) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
									v1->getReducedEventType(), Symbol(L"same-trigger"));
			return 1;
		}

		// I'd like to actually see if they come from the same derivational root,
		//  but I'll settle for, do they start with the same letters
		std::wstring str1 = stem1.to_string();
		std::wstring str2 = stem2.to_string();
		if (wcscmp(str1.substr(0,3).c_str(), str2.substr(0,3).c_str()) == 0) {
			resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
									v1->getReducedEventType(), Symbol(L"similar-trigger"));
			return 1;
		}

		return 0;
	}
};

#endif
