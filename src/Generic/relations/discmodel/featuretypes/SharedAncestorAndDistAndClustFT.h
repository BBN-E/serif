// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SHARED_ANCESTOR_DIST_CLUST_FT_H
#define SHARED_ANCESTOR_DIST_CLUST_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigram2IntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class SharedAncestorAndDistAndClustFT : public P1RelationFeatureType {
public:
	SharedAncestorAndDistAndClustFT() : P1RelationFeatureType(Symbol(L"shared-ancestor-dist-clust")) {}
	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigram2IntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0, 0 );
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
		wchar_t buffer[10];
		Symbol m1distSym;
		Symbol m2distSym;
		if(m1dist < 5){
#ifdef _WIN32
			m1distSym = Symbol(_itow(m1dist,buffer,10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", m1dist);
			m1distSym = Symbol(buffer);
#endif
		}
		else{
			m1distSym = Symbol(L">=5");
		}
		if(m2dist < 5){
#ifdef _WIN32
			m2distSym = Symbol(_itow(m1dist,buffer,10));
#else
			swprintf (buffer, sizeof(buffer)/sizeof(buffer[0]),
				  L"%d", m1dist);
			m2distSym = Symbol(buffer);
#endif
		}
		else{
			m2distSym = Symbol(L">=5");
		}
		
		WordClusterClass wc1(ment1->getNode()->getHeadWord());
		WordClusterClass wc2(ment2->getNode()->getHeadWord());


		int nfeatures = 0;
		if (wc1.c20() != 0 && wc2.c20() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
			m1distSym, m2distSym, wc1.c20(), wc2.c20());
		if (wc1.c16() != 0 && wc2.c16() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				m1distSym, m2distSym, wc1.c16(), wc2.c16());
		if (wc1.c12() != 0 && wc2.c12() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				m1distSym, m2distSym, wc1.c12(), wc2.c12());
		if (wc1.c8() != 0 && wc2.c8() != 0)
			resultArray[nfeatures++] = _new DTTrigram2IntFeature(this, state.getTag(),
				m1distSym, m2distSym, wc1.c8(), wc2.c8());
		return nfeatures;

								 

	}

};

#endif




















