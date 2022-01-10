// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_NEXT_WC_16_FT_H
#define HEADWORD_NEXT_WC_16_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/common/SymbolUtilities.h"


class HeadwordNextWC16FT : public P1DescFeatureType {
public:
	HeadwordNextWC16FT() : P1DescFeatureType(Symbol(L"headword-next-wc-16")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		int nfeatures = 0;
		Symbol nextword =  Symbol(L"-LASTWORD-");
		Symbol hw = o->getNode()->getHeadWord();

		int end = o->getNode()->getHeadPreterm()->getEndToken();
		if((end +1) < o->getNSentWords()){
			nextword  = o->getSentWord(end + 1);
		}

		if(isClusterableWord(nextword)){
			const WordClusterClass nextWC = WordClusterClass(SymbolUtilities::stemDescriptor(nextword), true);
			resultArray[nfeatures++] = _new DTIntFeature(this, state.getTag(), nextWC.c16());
		}

		return nfeatures;
	}

private:
	bool isClusterableWord(Symbol word) const {
		const wchar_t* wordstr = word.to_string();
		int len = static_cast<int>(wcslen(wordstr));
		bool foundlet = false;
		for(int i =0; i< len; i++){
			if(iswalpha(wordstr[i]) != 0){
				foundlet = true;
			}
		}
		return foundlet;
	}
};


#endif
