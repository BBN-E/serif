// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_ARG_CONFLICT_FEATURE_TYPE_H
#define EL_ARG_CONFLICT_FEATURE_TYPE_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"


class ELArgConflictFeatureType : public EventLinkFeatureType {
public:
	ELArgConflictFeatureType() : EventLinkFeatureType(Symbol(L"arg-conflict")) {}

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

		int same_count = 0;
		int different_count = 0;
		Symbol sameArray[50];
		Symbol differentArray[50];
		for (int i = 0; i < v1->getNArgs(); i++) {
			bool same_entity = false;
			bool different_entity = false;
			for (int j = 0; j < v2->getNArgs(); j++) {
				if (v1->getNthArgRole(i) == v2->getNthArgRole(j)) {
					if (o->getNthArgEntity(v1, i) == o->getNthArgEntity(v2, j))
						same_entity = true;
					else 
						different_entity = true;
				}
			}
			if (same_entity) 
				insertInSet(sameArray, same_count, v1->getNthArgRole(i));
			if (different_entity)
				insertInSet(differentArray, different_count, v1->getNthArgRole(i));
			if (same_count >= 50 || different_count >= 50)
					break;
		}

		int count = 0;
		for (int k = 0; k < different_count; k++) {
			bool same_found = false;
			for (int m = 0; m < same_count; m++) {
				if (differentArray[k] == sameArray[m]) {
					same_found = true;
					break;
				}
			}
			if (!same_found) {
				if (count >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION) {
						SessionLogger::warn("DT_feature_limit") 
							<<"ELArgConflictFeatureType discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
						break;
					}else{
						resultArray[count++] = _new DTTrigramFeature(this, state.getTag(), 
							v1->getEventType(),	differentArray[k]);
				}
			}
		}

		return count;
	}

protected:
	virtual void insertInSet(Symbol *set, int& n_elements, Symbol newElem) const {
		for (int i = 0; i < n_elements; i++) {
			if (set[i] == newElem)
				return;
		}
		set[n_elements++] = newElem;
	}
};

#endif
