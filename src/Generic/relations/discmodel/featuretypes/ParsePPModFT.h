// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_PP_MOD_FT_H
#define PARSE_PP_MOD_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/parse/STags.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class ParsePPModFT : public P1RelationFeatureType {
public:
	/*
	static void dumpANode(const SynNode* node){
		std::cout<<"Start of Node: "<<node->getTag().to_debug_string()
			<<" "<<node->getStartToken()<<" "<<node->getEndToken()<<std::endl;
		//node->dump(std::cout, 0);
		std::cout<<node->toDebugString(0);
		std::cout<<"\n***********End of Node***************"<<std::endl;
	}
	static void printALineBreak(){
		std::cout<<"\n\n------------------------\n\n"<<std::endl;
	}
	*/
	ParsePPModFT() : P1RelationFeatureType(Symbol(L"parse-pp-mod")) {}
	static bool hasPPForm(const SynNode* coveringnode, const SynNode* node1, 
		const SynNode* node2, Symbol& prep)
	{
		if(!((coveringnode->getTag() == STags::getNP()) ||(
			(coveringnode->getTag() == STags::getNPA()))))
		{
			return false;
		}
		for(int i=0; i< coveringnode->getNChildren(); i++){
			if(coveringnode->getChild(i)->isAncestorOf(node1)){
				if(node1->getHeadPreterm() != coveringnode->getHeadPreterm()){
					return false;
				}
				if(i+1 >= coveringnode->getNChildren()){
					return false;
				}
				const SynNode* pp = coveringnode->getChild(i+1);
				if(pp->getTag() != STags::getPP()){
					return false;
				}
				if(pp->getNChildren() < 2){
					return false;
				}
				if(!pp->getChild(1)->isAncestorOf(node2)){
					return false;
				}
				if(node2->getHeadPreterm() == pp->getChild(1)->getHeadPreterm()){
					prep = pp->getChild(0)->getHeadWord();
					return true;
				}
				return false;
			}
		}
		return false;
	}


	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
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
			throw UnexpectedInputException("ParsePPModFT::extractFeatures()", 
				"ment1 == 0");
		}
		if(ment2Node == 0){
			throw UnexpectedInputException("ParsePPModFT::extractFeatures()", 
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
			if(coveringNode !=0){
				Symbol prep;
				if(hasPPForm(coveringNode, ment1NodeP2, ment2NodeP2, prep)){
					resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
								prep,
								  o->getMention1()->getEntityType().getName(), 
								  o->getMention2()->getEntityType().getName());
					return 1;
				}
			}
		}
		else if(m2start < m1start ){
			coveringNode = secondaryParse->getCoveringNodeFromTokenSpan(m2start, m2end);
			if(coveringNode !=0){
				Symbol prep;
				if(hasPPForm(coveringNode, ment2NodeP2, ment1NodeP2, prep)){

					resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
								prep,
								  o->getMention2()->getEntityType().getName(), 
								  o->getMention1()->getEntityType().getName());
					return 1;
				}
			}
		}

		return 0;
	}

};

#endif
