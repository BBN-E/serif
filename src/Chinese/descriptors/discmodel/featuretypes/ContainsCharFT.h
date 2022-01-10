// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_CONTAINS_CHAR_FT_H
#define CH_CONTAINS_CHAR_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class ChineseContainsCharFT : public P1DescFeatureType {
public:
	ChineseContainsCharFT() : P1DescFeatureType(Symbol(L"contains-char")) {}

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

		for (int i = 0; i < len; i++) {
			if (i >= DTFeatureType::MAX_FEATURES_PER_EXTRACTION)
					return i;
			resultArray[i] = _new DTBigramFeature(this, state.getTag(),
														Symbol(&(str[i])));
		}
		return len;
	}

};

#endif
