// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PL_TYPE_PARENT_WORD_WC_FT_H
#define CH_PL_TYPE_PARENT_WORD_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/PronounLinkerUtils.h"
#include "Generic/WordClustering/WordClusterClass.h"

#include "Generic/theories/Mention.h"


class ChinesePLTypeParentWordWCFT : public DTCorefFeatureType {
public:
	ChinesePLTypeParentWordWCFT() : DTCorefFeatureType(Symbol(L"type-parent-word-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, 
											SymbolConstants::nullSymbol,
											0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Mention *ment = o->getMention();
	
		Symbol type = o->getEntity()->getType().getName();
		// tack on a possessive marker if it applies
		type = PronounLinkerUtils::augmentIfPOS(type, ment->getNode());

		Symbol parentWord = PronounLinkerUtils::getAugmentedParentHeadWord(ment->getNode());
		WordClusterClass parentWC = WordClusterClass(parentWord);
		
		int n_results = 0;

		if (parentWC.c8() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), 
															 type, parentWC.c8());
		if (parentWC.c12() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), 
															 type, parentWC.c12());
		if (parentWC.c16() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), 
															 type, parentWC.c16());
		if (parentWC.c20() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), 
															 type, parentWC.c20());
		return n_results;
			
	}

};
#endif
