// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_SAME_ARG_RED_FEATURE_TYPE_H
#define EL_SAME_ARG_RED_FEATURE_TYPE_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/theories/EventMention.h"


class ELSameArgRedFeatureType : public EventLinkFeatureType {
public:
	ELSameArgRedFeatureType() : EventLinkFeatureType(Symbol(L"same-arg-red")) {}

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

		int count = 0;
		bool discard = false;
		for (int i = 0; i < v1->getNArgs(); i++) {
			for (int j = 0; j < v2->getNArgs(); j++) {
				if (v1->getNthArgRole(i) == v2->getNthArgRole(j) &&
					o->getNthArgEntity(v1, i) == o->getNthArgEntity(v2, j))
				{
					if (count >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
						discard = true;
						break;
					}else{
						resultArray[count++] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getReducedEventType(),	v1->getNthArgRole(i));
					}
				}
			}
		}
		if (discard) {
			SessionLogger::warn("DT_feature_limit") 
				<<"ELSameArgRedFeatureType discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
		}
		return count;
	}
};

#endif
