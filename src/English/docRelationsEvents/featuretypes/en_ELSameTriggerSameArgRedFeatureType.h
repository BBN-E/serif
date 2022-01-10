// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_EL_SAME_TRIGGER_SAME_ARG_RED_FEATURE_TYPE_H
#define EN_EL_SAME_TRIGGER_SAME_ARG_RED_FEATURE_TYPE_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"


class EnglishELSameTriggerSameArgRedFeatureType : public EventLinkFeatureType {
public:
	EnglishELSameTriggerSameArgRedFeatureType() : EventLinkFeatureType(Symbol(L"same-trigger-same-arg-red")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
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

		Symbol v1Trigger = v1->getAnchorNode()->getHeadWord();
		Symbol v1Tag = v1->getAnchorNode()->getHeadPreterm()->getTag();
		Symbol v2Trigger = v2->getAnchorNode()->getHeadWord();
		Symbol v2Tag = v2->getAnchorNode()->getHeadPreterm()->getTag();

		Symbol stem1 = SymbolUtilities::stemWord(v1Trigger, v1Tag);
		Symbol stem2 = SymbolUtilities::stemWord(v2Trigger, v2Tag);

		int count = 0;
		bool discard = false;

		if (stem1 == stem2) {
			for (int i = 0; i < v1->getNArgs(); i++) {
				if (discard) break;

				for (int j = 0; j < v2->getNArgs(); j++) {
					if (v1->getNthArgRole(i) == v2->getNthArgRole(j) &&
						o->getNthArgEntity(v1, i) == o->getNthArgEntity(v2, j))
					{
						if (count >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
								discard = true;
								break;
						}
						resultArray[count++] = _new DTQuadgramFeature(this, state.getTag(), 
							v1->getReducedEventType(), Symbol(L"same-trigger"), v1->getNthArgRole(i));
					}
				}
			}

		}else
		{

		// I'd like to actually see if they come from the same derivational root,
		//  but I'll settle for, do they start with the same letters
		std::wstring str1 = stem1.to_string();
		std::wstring str2 = stem2.to_string();
		if (wcscmp(str1.substr(0,3).c_str(), str2.substr(0,3).c_str()) == 0) {			
	
			for (int i = 0; i < v1->getNArgs(); i++) {
				if (discard) break;

				for (int j = 0; j < v2->getNArgs(); j++) {
					if (v1->getNthArgRole(i) == v2->getNthArgRole(j) &&
						o->getNthArgEntity(v1, i) == o->getNthArgEntity(v2, j))
					{
						if (count >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
								discard = true;
								break;
						}
						resultArray[count++] = _new DTQuadgramFeature(this, state.getTag(), 
							v1->getReducedEventType(),	Symbol(L"similar-trigger"), v1->getNthArgRole(i));
					}
				}
			}
		}
		}
		if (discard) {
			SessionLogger::warn("DT_feature_limit") <<"en_ELSameTriggerSameArgRedFeatureType discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}
		return count;
	}
};

#endif
