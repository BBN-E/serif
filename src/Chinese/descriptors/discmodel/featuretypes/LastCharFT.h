// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_LAST_CHAR_FT_H
#define CH_LAST_CHAR_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class ChineseLastCharFT : public P1DescFeatureType {
public:
	ChineseLastCharFT() : P1DescFeatureType(Symbol(L"last-char")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		Symbol head = o->getNode()->getHeadWord();
		const wchar_t* str = head.to_string();
		int len = static_cast<int>(wcslen(str));
		Symbol last = Symbol(&(str[len-1]));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), last);
		return 1;
	}

};

#endif
