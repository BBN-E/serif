// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_AA_CAND_LAST_CHAR_FEATURE_TYPE_H
#define CH_AA_CAND_LAST_CHAR_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class ChineseAACandLastCharFeatureType : public EventAAFeatureType {
public:
	ChineseAACandLastCharFeatureType() : EventAAFeatureType(Symbol(L"cand-last-char")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		int d = o->getDistance();
		if (d > 10)
			return 0;

		Symbol word = o->getCandidateHeadword();
		const wchar_t* str = word.to_string();
		int len = static_cast<int>(wcslen(str));
		Symbol last = Symbol(&(str[len-1]));

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
			o->getReducedEventType(), last);
		return 1;
	}
};

#endif
