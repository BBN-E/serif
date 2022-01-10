// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_POSS_MOD_FT_H
#define AR_POSS_MOD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/parse/ar_STags.h"

class ArabicPossModFT : public P1RelationFeatureType {
public:
	ArabicPossModFT() : P1RelationFeatureType(Symbol(L"poss-mod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		if(o->getSecondaryParse() == 0){
			throw UnexpectedInputException("ParsePPModFT::extractFeatures()", 
				"ParsePPModFT used without a secondary parse");
		}
		

		const Mention* ment1 = o->getMention1();
		const Mention* ment2 = o->getMention2();
		const SynNode* ment1Node = ment1->getHead();
		const SynNode* ment2Node = ment2->getHead();
		if(ment1Node == 0){
			throw UnexpectedInputException("ArabicPossModFT::extractFeatures()", 
				"ment1 == 0");
		}
		if(ment2Node == 0){
			throw UnexpectedInputException("ArabicPossModFT::extractFeatures()", 
				"ment2 == 0");
		}
		int m1start = ment1Node->getStartToken();
		int m1end = ment1Node->getEndToken();
		int m2start = ment2Node->getStartToken();
		int m2end = ment2Node->getEndToken();

		if((m1end+1) == m2start){
			if((ment2->getMentionType() == Mention::PRON)){
				if(ArabicSTags::isPossPronoun(ment2Node->getTag())){
					resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
						ment1->getEntityType().getName(), ment2->getEntityType().getName());
					resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
						ment1->getHead()->getHeadWord(), ment2->getHead()->getHeadWord());
					return 2;
				}
				}
		}
		else if((m2end+1) == m1start) {
			if((ment1->getMentionType() == Mention::PRON) &&
				ArabicSTags::isPossPronoun(ment2Node->getTag())){
					resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
						ment1->getEntityType().getName(), ment2->getEntityType().getName());
					resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
						ment1->getHead()->getHeadWord(), ment2->getHead()->getHeadWord());
					return 2;

				}
		}
		return 0;
	}

};

#endif
