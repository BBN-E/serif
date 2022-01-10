// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_FT_H
#define HEADWORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class HeadwordFT : public P1DescFeatureType {
public:
	HeadwordFT() : P1DescFeatureType(Symbol(L"headword")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getNode()->getHeadWord());
		return 1;
	}

};

#endif
