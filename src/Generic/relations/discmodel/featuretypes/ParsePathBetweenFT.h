// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARSE_PATH_BTWN_FT_H
#define PARSE_PATH_BTWN_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class ParsePathBetweenFT : public P1RelationFeatureType {
public:
	ParsePathBetweenFT() : P1RelationFeatureType(Symbol(L"parse-path-btwn")) {}
	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
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
		Symbol m1Path;
		Symbol m2Path; 
		const Mention* ment1 = o->getMention1();
		const Mention* ment2 = o->getMention2();

		int ret = getParsePath(ment1, ment2, secondaryParse, m1Path, m2Path);
		if(ret == 0){
				return 0;
		}

		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), 
			m1Path, m2Path, ment1->getEntityType().getName(), 
			ment2->getEntityType().getName());
		return 1;
								 

	}
	static int getParsePath(const Mention* ment1, const Mention* ment2, const SynNode* secondaryParse, Symbol& m1Path, Symbol& m2Path){
	
		const SynNode* ment1Node = ment1->getHead();
		const SynNode* ment2Node = ment2->getHead();
		m1Path = Symbol(L"-NONE-");
		m2Path = Symbol(L"-NONE-");
		if(ment1Node == 0){
			throw UnexpectedInputException("SharedAncestorAndDistFT::extractFeatures()", 
				"ment1 == 0");
		}
		if(ment2Node == 0){
			throw UnexpectedInputException("SharedAncestorAndDistFT::extractFeatures()", 
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
			return 0;
		}
		if(ment2NodeP2 == 0){
			std::cout<<"Error: empty ment2NodeP2"<<std::endl;
			return 0;
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
		int m1dist = ment1NodeP2->getAncestorDistance(coveringNode);
		int m2dist = ment2NodeP2->getAncestorDistance(coveringNode);

		//paths >5, not interesting ???
		if((m1dist > 5) || (m2dist >5)){
			return 0;
		}
		const int buffsize = 150;

		wchar_t buffer[buffsize];
		int remaining_buffsize = buffsize - 1;
		wcscpy(buffer, L"path:");
		remaining_buffsize -= 5;
		const SynNode* temp = ment1NodeP2;
		while(temp != coveringNode){
			wcsncat(buffer, temp->getTag().to_string(),remaining_buffsize);
			remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
			if (remaining_buffsize <= 0)
				break;
			wcsncat(buffer, L"_", remaining_buffsize);
			remaining_buffsize -= 1;
			if (remaining_buffsize <= 0)
				break;
			temp = temp->getParent();
			if(temp == 0){
				return 0;
			}
		}
		//include coveringnode tag
		if (remaining_buffsize > 0) {
			wcsncat(buffer, temp->getTag().to_string(),remaining_buffsize);
			remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
		} 
		if (remaining_buffsize > 0) {
			wcsncat(buffer, L"_", remaining_buffsize);
			remaining_buffsize -= 1;
		}
		m1Path = Symbol(buffer);


		remaining_buffsize = buffsize - 1;
		wcscpy(buffer, L"path:");
		remaining_buffsize -= 5;
		temp = ment2NodeP2;
		while(temp != coveringNode){
			wcsncat(buffer, temp->getTag().to_string(),remaining_buffsize);
			remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
			if (remaining_buffsize <= 0)
				break;
			wcsncat(buffer, L"_", remaining_buffsize);
			remaining_buffsize -= 1;
			if (remaining_buffsize <= 0)
				break;
			temp = temp->getParent();
			if(temp == 0){
				return 0;
			}
		}

		//include coveringnode tag
		if (remaining_buffsize > 0) {
			wcsncat(buffer, temp->getTag().to_string(),remaining_buffsize);
			remaining_buffsize -= (int) wcslen(temp->getTag().to_string());
		} 
		if (remaining_buffsize > 0) {
			wcsncat(buffer, L"_", remaining_buffsize);
			remaining_buffsize -= 1;
		}
		m2Path = Symbol(buffer);

		return 1;
	}

};

#endif
