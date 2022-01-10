// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_DISTANCE_FT_H
#define AR_DISTANCE_FT_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "relations/discmodel/P1RelationFeatureType.h"
#include "discTagger/DTIntFeature.h"
#include "discTagger/DTState.h"
#include "relations/discmodel/RelationObservation.h"
#include "theories/MentionSet.h"
#include "relations/RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"

class ArabicMentionDistanceFT : public P1RelationFeatureType {
public:
	ArabicMentionDistanceFT() : P1RelationFeatureType(Symbol(L"mention-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1000);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		//std::cerr << "Entered ArabicMentionDistanceFT::extractFeatures()\n";
		//std::cerr.flush();
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		//std::cerr << "Got the observation\n";
		//std::cerr.flush();
		const SynNode *head1 = o->getMention1()->getNode()->getHeadPreterm();
		const SynNode *head2 = o->getMention2()->getNode()->getHeadPreterm();
		//std::cerr << "Found both head preterminals\n";
		//std::cerr.flush();
		int tok1 = head1->getStartToken();
		int tok2 = head2->getStartToken();
		//std::cerr << "Got tokens " << tok1 << " and " << tok2 << "\n";
		//std::cerr.flush();
		resultArray[0] = _new DTIntFeature(this, state.getTag(), tok2 - tok1 - 1);	
		//std::cerr << "Built feature\n"; 
		//std::cerr.flush();
		return 1;

	}

};

#endif
