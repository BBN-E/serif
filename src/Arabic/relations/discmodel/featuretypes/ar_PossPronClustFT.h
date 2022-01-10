// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_POSS_PRON_CLUST_FT_H
#define AR_POSS_PRON_CLUST_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/WordConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"


class ArabicPossPronClustFT : public P1RelationFeatureType {
public:
	ArabicPossPronClustFT() : P1RelationFeatureType(Symbol(L"poss-pron-clust")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		
		const SynNode* pronnode = 0;
		const Mention* otherment = 0;
		const Mention* pronment = 0;
		if(ArabicSTags::isPossPronoun(o->getMention1()->getNode()->getHeadPreterm()->getTag())){
			pronnode = o->getMention1()->getNode()->getHeadPreterm();
			pronment = o->getMention1();
			otherment = o->getMention2();
		}
		else if(ArabicSTags::isPossPronoun(o->getMention2()->getNode()->getHeadPreterm()->getTag()))
		{
			pronnode = o->getMention2()->getNode()->getHeadPreterm();
			pronment = o->getMention2();
			otherment = o->getMention1();
		}
		else{
			return 0;
		}
		if(ArabicSTags::isPronoun(otherment->getNode()->getHeadPreterm()->getTag())){
			return 0;
		}
		//check the distance (poss pron must directly follow mention)
		int dist = RelationUtilities::get()->calcMentionDist(o->getMention1(), o->getMention2());
		if(dist != 1){
			return 0;
		}
		if(pronnode->getStartToken() < otherment->getNode()->getStartToken()){
			return 0;
		}

		Symbol head1 = otherment->getNode()->getHeadWord();
		Symbol head2 = pronnode->getHeadWord();
		Symbol enttype1 = otherment->getEntityType().getName();
		Symbol enttype2 = pronment->getEntityType().getName();
		

		WordClusterClass wc(head1);
		resultArray[0] = _new DTBigramIntFeature(this, state.getTag(),
			head2, wc.c8());
		resultArray[1] = _new DTBigramIntFeature(this, state.getTag(),
			head2, wc.c12());
		resultArray[2] = _new DTBigramIntFeature(this, state.getTag(),
			head2, wc.c16());
		resultArray[3] = _new DTBigramIntFeature(this, state.getTag(),
			head2, wc.c20());
		resultArray[4] = _new DTBigramIntFeature(this, state.getTag(),
			Symbol(L"-POSS_PRON-"), wc.c8());
		resultArray[5] = _new DTBigramIntFeature(this, state.getTag(),
			Symbol(L"-POSS_PRON-"), wc.c12());
		resultArray[6] = _new DTBigramIntFeature(this, state.getTag(),
			Symbol(L"-POSS_PRON-"), wc.c16());
		resultArray[7] = _new DTBigramIntFeature(this, state.getTag(),
			Symbol(L"-POSS_PRON-"), wc.c20());


		return 8;


	}

};

#endif
