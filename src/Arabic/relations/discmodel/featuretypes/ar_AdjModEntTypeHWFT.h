// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ADJ_ENTTYPE_HW_FT_H
#define AR_ADJ_ENTTYPE_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class ArabicAdjModEntTypeHWFT : public P1RelationFeatureType {
public:
	ArabicAdjModEntTypeHWFT() : P1RelationFeatureType(Symbol(L"adj-mod-enttype-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol , SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		//std::cerr << "Entered POSBetweenTypesFT::extractFeatures()\n";
		//std::cerr.flush();
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));


		Symbol mtype1 = Symbol(L"CONFUSED");
		if (o->getMention1()->getMentionType() == Mention::NAME)
			mtype1 = Symbol(L"NAME");
		else if (o->getMention1()->getMentionType() == Mention::DESC)
			mtype1 = Symbol(L"DESC");
		else if (o->getMention1()->getMentionType() == Mention::PRON)
			mtype1 = Symbol(L"PRON");

		Symbol mtype2 = Symbol(L"CONFUSED");
		if (o->getMention2()->getMentionType() == Mention::NAME)
			mtype2 = Symbol(L"NAME");
		else if (o->getMention2()->getMentionType() == Mention::DESC)
			mtype2 = Symbol(L"DESC");
		else if (o->getMention2()->getMentionType() == Mention::PRON)
			mtype2 = Symbol(L"PRON");
		
		int dist = RelationUtilities::get()->calcMentionDist(o->getMention1(), o->getMention2());
		int nresults = 0;
		if (dist == 1) {
				const SynNode* preterm = o->getMention1()->getNode()->getHeadPreterm();

				resultArray[nresults++] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), mtype1,
								  o->getMention2()->getNode()->getHeadWord(), mtype2);
				resultArray[nresults++] = _new DTQuintgramFeature(this, state.getTag(),
								  o->getMention1()->getNode()->getHeadWord(), mtype1,
								  o->getMention2()->getEntityType().getName(),  mtype2);


				return nresults;

		}
		return 0;
		
	}

};

#endif
