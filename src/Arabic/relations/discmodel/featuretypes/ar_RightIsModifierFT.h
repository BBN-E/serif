// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_IS_MODIFIER_FT_H
#define AR_RIGHT_IS_MODIFIER_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"

class ArabicRightIsModifierFT : public P1RelationFeatureType {
public:
	ArabicRightIsModifierFT() : P1RelationFeatureType(Symbol(L"right-is-mod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		const SynNode *chunk = RelationUtilities::get()->findNPChunk(o->getMention2()->getNode());
		
		if (chunk->getHeadWord() != o->getMention2()->getNode()->getHeadWord()) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(),  
												  o->getMention2()->getEntityType().getName());
			return 1;
		}

		return 0;

	}

};

#endif
