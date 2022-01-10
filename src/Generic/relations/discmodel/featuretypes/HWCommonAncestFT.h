// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_COMMOM_ANCESTOR_FT_H
#define HW_COMMOM_ANCESTOR_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"

	  
class HWCommonAncestFT : public P1RelationFeatureType {
public:

	HWCommonAncestFT() : P1RelationFeatureType(Symbol(L"hw-common-ancestor")) {}
	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol );
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		const SynNode* secondaryParse;
		if(o->getSecondaryParse() == 0){
			secondaryParse = o->getParse()->getRoot();
		} else secondaryParse = o->getSecondaryParse()->getRoot();
		

		const Mention* ment1 = o->getMention1();
		const Mention* ment2 = o->getMention2();
		const SynNode* ment1Node = ment1->getHead();
		const SynNode* ment2Node = ment2->getHead();
		if(ment1Node == 0){
			throw UnexpectedInputException("HWCommonAncest::extractFeatures()", 
				"ment1 == 0");
		}
		if(ment2Node == 0){
			throw UnexpectedInputException("HWCommonAncest::extractFeatures()", 
				"ment2 == 0");
		}
		int m1start = ment1Node->getStartToken();
		int m1end = ment1Node->getEndToken();
		int m2start = ment2Node->getStartToken();
		int m2end = ment2Node->getEndToken();

		const SynNode* ment1NodeP2 = secondaryParse->getNodeByTokenSpan(m1start, m1end);
		const SynNode* ment2NodeP2 = secondaryParse->getNodeByTokenSpan(m2start, m2end);
		if((ment1NodeP2 ==0) && (ment1->mentionType ==Mention::PRON)){
			ment1Node = getPronHeadNode(ment1Node);
			m1start = ment1Node->getStartToken();
			m1end = ment1Node->getEndToken();
			ment1NodeP2 = secondaryParse->getNodeByTokenSpan(m1start, m1end);

		}
		if((ment2NodeP2 ==0) && (ment2->mentionType ==Mention::PRON)){
			ment2Node = getPronHeadNode(ment1Node);
			m2start = ment1Node->getStartToken();
			m2end = ment1Node->getEndToken();
			ment2NodeP2 = secondaryParse->getNodeByTokenSpan(m2start, m2end);

		}
		if(ment1NodeP2 == 0){
			std::cout<<"Error: empty ment1NodeP2"<<std::endl;
		}
		if(ment2NodeP2 == 0){
			std::cout<<"Error: empty ment2NodeP2"<<std::endl;
		}
		const SynNode* coveringNode = 0;
		if(m1start < m2start){
			coveringNode = secondaryParse->getCoveringNodeFromTokenSpan(m1start, m2end);
		}
		else if(m2start < m1start ){
			coveringNode = secondaryParse->getCoveringNodeFromTokenSpan(m2start, m2end);
		}
		if(coveringNode == 0){
			return 0;
		}
		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), 
			coveringNode->getHeadWord(), coveringNode->getTag(),
			ment1->getEntityType().getName(), ment2->getEntityType().getName());
		return 1;

	}

};

#endif
