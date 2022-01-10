// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NOT_PERSON_HYPONYM_FT_H
#define EN_NOT_PERSON_HYPONYM_FT_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DT2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"
#include "Generic/wordnet/xx_WordNet.h"


class EnglishNotPersonHyponymFT : public P1DescFeatureType {
public:
	EnglishNotPersonHyponymFT() : P1DescFeatureType(Symbol(L"not-person-hyponym")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));

		int nfeatures = 0;

		if (o->getNOffsets() == 0)
			return 0;

		if (!o->isPersonHyponym()) {
			resultArray[nfeatures++] = _new DTBigramFeature(this, state.getTag(), 
				Symbol(L"IN-WORDNET-BUT-NOT-PERSON-HYPONYM"));
		}

		return nfeatures;
	}

};

#endif
