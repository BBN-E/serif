// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_STEMMED_HEADWORD_FT_H
#define EN_STEMMED_HEADWORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordnet/xx_WordNet.h"


class EnglishStemmedHeadwordFT : public P1DescFeatureType {
public:
	EnglishStemmedHeadwordFT() : P1DescFeatureType(Symbol(L"stemmed-headword")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
			WordNet::getInstance()->stem_noun(o->getNode()->getHeadWord()));
		return 1;
	}

};

#endif
