// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ADJNAME_ENTTYPE_FT_H
#define ADJNAME_ENTTYPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/theories/SynNode.h"
#include "Generic/descriptors/discmodel/P1DescFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/descriptors/discmodel/DescriptorObservation.h"


class AdjNameEntTypeFT : public P1DescFeatureType {
public:
	AdjNameEntTypeFT() : P1DescFeatureType(Symbol(L"adj-name-enttype-ft")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DescriptorObservation *o = static_cast<DescriptorObservation*>(
			state.getObservation(0));
		int starttok = o->getNode()->getHeadPreterm()->getStartToken();
		int endtok = o->getNode()->getHeadPreterm()->getEndToken();
		Mention* adjFollowingMent = 0;
		Mention* adjPrecedingMent = 0;
		for(int i = 0; i< o->getMentionSet()->getNMentions(); i++){
			Mention* ment  = o->getMentionSet()->getMention(i);
			if(ment->getMentionType() == Mention::NAME){
				Mention* menChild = ment;
				const SynNode* head = menChild->node;
				while((menChild = menChild->getChild()) != 0)
					head = menChild->node;			
				if(head->getStartToken() == (endtok+1)){
					adjFollowingMent = ment;
				}
				else if(head->getEndToken() == (starttok - 1)){
					adjPrecedingMent = 0;
				}
			}
		}
		int nresults = 0;
		if(adjFollowingMent != 0){
			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(), 
				o->getNode()->getHeadWord(), adjFollowingMent->getEntityType().getName(), 
				Symbol(L"NEXT"));

			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(), 
				Symbol(L"-BACKOFF-"), adjFollowingMent->getEntityType().getName(), 
				Symbol(L"NEXT"));
		}
		if(adjPrecedingMent != 0){
			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(), 
				o->getNode()->getHeadWord(), adjPrecedingMent->getEntityType().getName(), 
				Symbol(L"-PREV-"));

			resultArray[nresults++] = _new DTQuadgramFeature(this, state.getTag(), 
				Symbol(L"-BACKOFF-"), adjPrecedingMent->getEntityType().getName(), 
				Symbol(L"-PREV-"));
		}
		return nresults;
	}

};


#endif
