// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_TRIGRAM_FT_H
#define HEADWORD_TRIGRAM_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class HWTrigramFT : public P1DescFeatureType {
public:
	HWTrigramFT() : P1DescFeatureType(Symbol(L"hw-trigram")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		Symbol prevword1 = Symbol(L"-FIRSTWORD-");
		Symbol prevword2 = Symbol(L"-FIRSTWORD-");
		Symbol nextword1 = Symbol(L"-LASTWORD-");
		Symbol nextword2 = Symbol(L"-LASTWORD-");
		int start = o->getNode()->getHeadPreterm()->getStartToken();
		int end = o->getNode()->getHeadPreterm()->getEndToken();
		if( start > 0){
			prevword1  = o->getSentWord(start - 1);
		}
		if( start > 1){
			prevword2  = o->getSentWord(start - 2);
		}
		if((end +1) < o->getNSentWords()){
			nextword1  = o->getSentWord(end + 1);
		}
		if((end +2) < o->getNSentWords()){
			nextword1  = o->getSentWord(end + 2);
		}

		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), 
			Symbol(L"-PREV-"), prevword2, prevword1, o->getNode()->getHeadWord());
		resultArray[1] = _new DTQuintgramFeature(this, state.getTag(),
			Symbol(L"-CENTER-"), prevword1, o->getNode()->getHeadWord(), nextword1);
		resultArray[2] = _new DTQuintgramFeature(this, state.getTag(),
			Symbol(L"-NEXT-"), o->getNode()->getHeadWord(), nextword1, nextword2);

		return 3;
	}

};


#endif
