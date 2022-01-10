// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_PREV_WORD_WC
#define HW_PREV_WORD_WC

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class HWPrevWordWCFT : public P1DescFeatureType {
public:
	HWPrevWordWCFT() : P1DescFeatureType(Symbol(L"hw-prev-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		int nfeatures = 0;
		Symbol hw = o->getNode()->getHeadWord();
		const SynNode* prev = o->getNode()->getHeadPreterm()->getPrevTerminal();
		if(prev != 0){
			Symbol prev_hw = prev->getHeadWord();
			WordClusterClass wc = WordClusterClass(prev_hw);
			if(wc.c8() != 0){
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(),
					hw, wc.c8());
			}
			if(wc.c12() != 0){
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(),
					hw, wc.c12());
			}
			if(wc.c16() != 0){
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(),
					hw, wc.c16());
			}
			if(wc.c20() != 0){
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, state.getTag(),
					hw, wc.c20());
			}
		}
		return nfeatures;
	}

};

#endif
