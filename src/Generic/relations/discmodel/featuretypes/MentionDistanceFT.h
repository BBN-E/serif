// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DISTANCE_FT_H
#define DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/relations/xx_RelationUtilities.h"

class MentionDistanceFT : public P1RelationFeatureType {
public:
	MentionDistanceFT() : P1RelationFeatureType(Symbol(L"mention-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1000);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		const SynNode *head1 = o->getMention1()->getNode()->getHeadPreterm();
		const SynNode *head2 = o->getMention2()->getNode()->getHeadPreterm();

		int tok1 = head1->getStartToken();
		int tok2 = head2->getStartToken();

		resultArray[0] = _new DTIntFeature(this, state.getTag(), tok2 - tok1 - 1);	
		return 1;

	}

};

#endif
