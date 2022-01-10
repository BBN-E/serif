// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_RIGHT_CLITIC_FT_H
#define AR_RIGHT_CLITIC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"
#include "Arabic/common/ar_WordConstants.h"

class ArabicRightCliticFT : public P1RelationFeatureType {
public:
	ArabicRightCliticFT() : P1RelationFeatureType(Symbol(L"right-clitic")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		
		Symbol clitic;
		bool has_clitic = false;

		Symbol head2 = o->getMention2()->getNode()->getHeadWord();

		// check to see if head is clitic
		if (ArabicWordConstants::isPrefixClitic(head2)) {
			has_clitic = true;
			clitic = head2;
		}
		// check previous word
		else {
			const SynNode *term = o->getMention2()->getNode()->getHeadPreterm()->getHead();
			const SynNode *prev = term->getPrevTerminal();
			if (prev != 0 && ArabicWordConstants::isPrefixClitic(prev->getHeadWord())) {
				has_clitic = true;
				clitic = prev->getHeadWord();
			}
		}

		if (has_clitic) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), clitic);
			return 1;
		}

		return 0;

	}

};

#endif
