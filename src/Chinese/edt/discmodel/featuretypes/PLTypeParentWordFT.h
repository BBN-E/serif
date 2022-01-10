// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_PL_TYPE_PARENT_WORD_FT_H
#define CH_PL_TYPE_PARENT_WORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/PronounLinkerUtils.h"

#include "Generic/theories/Mention.h"


class ChinesePLTypeParentWordFT : public DTCorefFeatureType {
public:
	ChinesePLTypeParentWordFT() : DTCorefFeatureType(Symbol(L"type-parent-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol);
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

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), type, parentWord);
		return 1;
		
		
	}

};
#endif
