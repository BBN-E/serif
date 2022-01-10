// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HWCLUST_COMMOM_ANCESTOR_FT_H
#define HWCLUST_COMMOM_ANCESTOR_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"

	  
class HWClustCommonAncestFT : public P1RelationFeatureType {
public:

	HWClustCommonAncestFT() : P1RelationFeatureType(Symbol(L"hwclust-common-ancestor")) {}
	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  0 );
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
			throw UnexpectedInputException("HWClustCommonAncestFT::extractFeatures()", 
				"ment1 == 0");
		}
		if(ment2Node == 0){
			throw UnexpectedInputException("HWClustCommonAncestFT::extractFeatures()", 
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



		WordClusterClass wc(coveringNode->getHeadWord());


		int nfeatures = 0;
		if (wc.c20() != 0 ){

			resultArray[nfeatures] = _new DTQuadgramIntFeature(this, state.getTag(), 
			coveringNode->getTag(),	ment1->getEntityType().getName(), 
			ment2->getEntityType().getName(), wc.c20());
			nfeatures++;
		}
		if (wc.c16() != 0 ){

			resultArray[nfeatures] = _new DTQuadgramIntFeature(this, state.getTag(), 
			coveringNode->getTag(),	ment1->getEntityType().getName(), 
			ment2->getEntityType().getName(), wc.c16());
			nfeatures++;
		}
		if (wc.c12() != 0 ){

			resultArray[nfeatures] = _new DTQuadgramIntFeature(this, state.getTag(), 
			coveringNode->getTag(),	ment1->getEntityType().getName(), 
			ment2->getEntityType().getName(), wc.c12());
			nfeatures++;
		}
		if (wc.c8() != 0 ){

			resultArray[nfeatures] = _new DTQuadgramIntFeature(this, state.getTag(), 
			coveringNode->getTag(),	ment1->getEntityType().getName(), 
			ment2->getEntityType().getName(), wc.c8());
			nfeatures++;
		}
		return nfeatures;

	}

};

#endif
