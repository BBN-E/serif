// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EL_SAME_CORE_NP_RED_FEATURE_TYPE_H
#define EL_SAME_CORE_NP_RED_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/EventLinkFeatureType.h"
#include "Generic/docRelationsEvents/EventLinkObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/EventMention.h"
#include "Generic/parse/LanguageSpecificFunctions.h"


class ELSameCoreNPRedFeatureType : public EventLinkFeatureType {
public:
	ELSameCoreNPRedFeatureType() : EventLinkFeatureType(Symbol(L"same-core-np-red")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventLinkObservation *o = static_cast<EventLinkObservation*>(
			state.getObservation(0));

		const SynNode *n1 = o->getVMention1()->getAnchorNode();
		const SynNode *n2 = o->getVMention2()->getAnchorNode();

		while (n1 != 0) {
			if (LanguageSpecificFunctions::isCoreNPLabel(n1->getTag()))
				break;
			n1 = n1->getParent();
		}

		while (n2 != 0) {
			if (LanguageSpecificFunctions::isCoreNPLabel(n2->getTag()))
				break;
			n2 = n2->getParent();
		}

		if (n1 == 0 || n2 == 0)
			return 0;

		std::wstring str1 = n1->toTextString();
		std::wstring str2 = n2->toTextString();

		if (wcscmp(str1.c_str(), str2.c_str()) != 0)
			return 0;

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
						o->getVMention1()->getReducedEventType());

		return 1;
	}
};

#endif
