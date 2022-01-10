// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_PAREN_GPE_FT_H
#define AR_PAREN_GPE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/relations/ar_RelationUtilities.h"
#include "Arabic/parse/ar_STags.h"
/*
This feature is primarily designed to learn the City -LRB- Country -RRB- construction
that occurs at the begining of many news articles, esp. AFA

(display problems mean that this construction look like (City Country) 
in an html viewer
*/
class ArabicParenGPEFT : public P1RelationFeatureType {
public:
	ArabicParenGPEFT() : P1RelationFeatureType(Symbol(L"paren-gpe")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		const Mention* m1 = o->getMention1();
		const Mention* m2 = o->getMention2();
		//lbnAn
		
		//must both be GPE names!
		if(!m1->getEntityType().matchesGPE())
			return 0;
		if(! m2->getEntityType().matchesGPE())
			return 0;
		if(m1->getMentionType() != Mention::NAME)
			return 0;
		if(m1->getMentionType() != Mention::NAME)
			return 0;

		//get the full name extent
		//name extent 1
		const SynNode* name1 = getNameNode(m1);
		//name extent2
		const SynNode* name2 = getNameNode(m2);

		//check the distance (poss pron must directly follow mention)
		bool paren1 = false;
		bool paren2 = false;
		int first_name_end = -1;
		int second_name_start = -1;
		Symbol open_paren = Symbol(L"-LRB-");
		Symbol close_paren = Symbol(L"-RRB-");
		const SynNode* next1 = 0;
		const SynNode* next2 = 0;
		
		if(name1->getStartToken() < name2->getStartToken()){
			next1 = name1->getNextTerminal();
			next2 = name2->getNextTerminal();
			first_name_end = name1->getEndToken();
			second_name_start = name2->getStartToken();
		}
		else{
			next1 = name2->getNextTerminal();
			next2 = name1->getNextTerminal();
			first_name_end = name2->getEndToken();
			second_name_start = name1->getStartToken();
		}
		if((next1 !=0) &&( (next1->getHeadWord() == open_paren) ))
		{
				paren1 = true;
		}
		if((second_name_start - first_name_end) != 2 ){
			return 0;
		}
	
		if((next2 !=0) &&( (next2->getHeadWord() == close_paren)))
		{
				paren2 = true;
		}
		if(paren1 && paren2){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
				Symbol(L"-BACKOFF-"));
			//if(m1->getSentenceNumber() == 0){
			//	resultArray[1] = _new DTBigramFeature(this, state.getTag(),  
			//									 Symbol(L"-FIRST_SENT"));
			//	return 2;
			//}
			return 1;
		}
		return 0;

	}
	static const SynNode* getNameNode(const Mention* ment){
		const Mention* menChild = ment;
		const SynNode* head = menChild->node;
		while((menChild = menChild->getChild()) != 0)
			head = menChild->node;			
		return head;
	}

};


#endif
