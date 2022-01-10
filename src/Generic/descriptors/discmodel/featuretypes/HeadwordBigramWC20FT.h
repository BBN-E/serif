// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEADWORD_BIGRAM_WC_20_FT_H
#define HEADWORD_BIGRAM_WC_20_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DT3IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/common/SymbolUtilities.h"


class HeadwordBigramWC20FT : public P1DescFeatureType {
public:
	HeadwordBigramWC20FT() : P1DescFeatureType(Symbol(L"headword-bigram-wc-20")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT3IntFeature(this, SymbolConstants::nullSymbol, 0, 0, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		
		int nfeatures = 0;
		Symbol prevword = Symbol(L"-FIRSTWORD-");
		Symbol nextword =  Symbol(L"-LASTWORD-");
		Symbol hw = o->getNode()->getHeadWord();

		int start = o->getNode()->getHeadPreterm()->getStartToken();
		int end = o->getNode()->getHeadPreterm()->getEndToken();
		if( start > 0){
			prevword  = o->getSentWord(start - 1);
		}
		if((end +1) < o->getNSentWords()){
			nextword  = o->getSentWord(end + 1);
		}

		if(isClusterableWord(hw)){
			const WordClusterClass hwWC = WordClusterClass(SymbolUtilities::stemDescriptor(hw), true);
			if(isClusterableWord(prevword)){
				const WordClusterClass prevWC = WordClusterClass(SymbolUtilities::stemDescriptor(prevword), true);
				resultArray[nfeatures++] = _new DT3IntFeature(this, state.getTag(), -1, prevWC.c20(), hwWC.c20());
			}
			if(isClusterableWord(nextword)){
				const WordClusterClass nextWC = WordClusterClass(SymbolUtilities::stemDescriptor(nextword), true);
				resultArray[nfeatures++] = _new DT3IntFeature(this, state.getTag(), 1, hwWC.c20(), nextWC.c20());
			}
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
