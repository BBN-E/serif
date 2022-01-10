// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PL_TYPE_PARENT_WORD_HW_FT_H
#define CH_PL_TYPE_PARENT_WORD_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/PronounLinkerUtils.h"

#include "Generic/theories/Mention.h"


class ChinesePLTypeParentWordHWFT : public DTCorefFeatureType {
public:
	ChinesePLTypeParentWordHWFT() : DTCorefFeatureType(Symbol(L"type-parent-word-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		
		Mention *ment = o->getMention();
		Symbol headWord = o->getMention()->getNode()->getHeadWord();
	
		Symbol type = o->getEntity()->getType().getName();
		// tack on a possessive marker if it applies
		type = PronounLinkerUtils::augmentIfPOS(type, ment->getNode());

		Symbol parentWord = PronounLinkerUtils::getAugmentedParentHeadWord(ment->getNode());

		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), type, 
													  parentWord, headWord);
		return 1;
		
		
	}

};
#endif
