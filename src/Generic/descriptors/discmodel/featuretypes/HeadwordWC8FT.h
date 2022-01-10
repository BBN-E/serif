// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_WC_8_FT_H
#define HEADWORD_WC_8_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class HeadwordWC8FT : public P1DescFeatureType {
public:
	HeadwordWC8FT() : P1DescFeatureType(Symbol(L"headword-wc-8")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		int nfeatures = 0;
		if (o->getHeadwordWC().c8() != 0) {
			resultArray[nfeatures++] = _new DTIntFeature(this, state.getTag(),
				o->getHeadwordWC().c8());
		}
		return nfeatures;
	}

};

#endif
