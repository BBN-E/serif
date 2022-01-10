// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_POSITION_IN_SENTENCE_FT_H
#define HEADWORD_POSITION_IN_SENTENCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/common/SymbolUtilities.h"


class HWPosInSentFT : public P1DescFeatureType {
public:
	HWPosInSentFT() : P1DescFeatureType(Symbol(L"hw-pos-in-sent")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		int hw_pos = o->getNode()->getHeadIndex();
		if(hw_pos >20)
			hw_pos = 20;

		resultArray[0] = _new DTIntFeature(this, state.getTag(), hw_pos);
		return 1;
	}

};


#endif
