// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_ADJ_ENTTYPE_CLUST_FT_H
#define AR_ADJ_ENTTYPE_CLUST_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigram2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"


class ArabicAdjModEntTypeClustFT : public P1RelationFeatureType {
public:
	ArabicAdjModEntTypeClustFT() : P1RelationFeatureType(Symbol(L"adj-mod-enttype-clust")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigram2IntFeature(this, SymbolConstants::nullSymbol, 
								SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								0, 0);
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
			int clust1[4];
			int clust2[4];
			RelationUtilities::get()->fillClusterArray(o->getMention1(), clust1);
			RelationUtilities::get()->fillClusterArray(o->getMention2(), clust2);
			for(int i =0; i< 4; i++){
				if(clust1[i] == 0){
					continue;
				}
				for(int j =0; j < 4; j++){
					if(clust2[j] == 0){
						continue;
					}
					resultArray[nresults++] = _new DTTrigram2IntFeature(this, state.getTag(),
								  o->getMention1()->getEntityType().getName(), 
								  o->getMention2()->getEntityType().getName(),
								  clust1[i], clust2[j]);
				}
			}
			return nresults;

		}
		return 0;
		
	}

};

#endif
