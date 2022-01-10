// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_BIGRAM_FT_H
#define HEADWORD_BIGRAM_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class HWBigramFT : public P1DescFeatureType {
public:
	HWBigramFT() : P1DescFeatureType(Symbol(L"hw-bigram")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		Symbol prevword = Symbol(L"-FIRSTWORD-");
		Symbol nextword =  Symbol(L"-LASTWORD-");
		int start = o->getNode()->getHeadPreterm()->getStartToken();
		int end = o->getNode()->getHeadPreterm()->getEndToken();
		if( start > 0){
			prevword  = o->getSentWord(start - 1);
		}
		if((end +1) < o->getNSentWords()){
			nextword  = o->getSentWord(end + 1);
		}


		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), Symbol(L"-PREV-"), prevword, o->getNode()->getHeadWord());
		resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), Symbol(L"-NEXT-"), o->getNode()->getHeadWord(), nextword);

		return 2;
	}

};


#endif
