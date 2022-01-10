// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_HEAD_WORD_FT_H
#define EN_HEAD_WORD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

class EnglishHeadWordFT : public P1RelationFeatureType {
public:
	EnglishHeadWordFT() : P1RelationFeatureType(Symbol(L"head-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		Symbol head1 = o->getMention1()->getNode()->getHeadWord();
		Symbol head2 = o->getMention2()->getNode()->getHeadWord();
		Symbol enttype1 = o->getMention1()->getEntityType().getName();
		Symbol enttype2 = o->getMention2()->getEntityType().getName();

		int tok1 = o->getMention1()->getNode()->getHeadPreterm()->getStartToken();
		int tok2 = o->getMention2()->getNode()->getHeadPreterm()->getStartToken();

		if (tok2 - tok1 >= 6)
			return 0;

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
			enttype1, head2);

		resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
			head1, enttype2);

		return 2;	
	}
};

#endif
