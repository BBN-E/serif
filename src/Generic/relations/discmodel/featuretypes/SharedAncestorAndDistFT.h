// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SHARED_ANCESTOR_DIST_FT_H
#define SHARED_ANCESTOR_DIST_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"
#include <boost/lexical_cast.hpp>

#define MAX_DISTANCE 5
// Note: EXTRA_DIST_SYMS is used to get around a bug
// how m2distSym is set -- see below for details.  Once
// this bug is fixed, EXTRA_DIST_SYMS can go away.
#define EXTRA_DIST_SYMS 256

class SharedAncestorAndDistFT : public P1RelationFeatureType {
private:
	Symbol _distanceSymbols[MAX_DISTANCE+EXTRA_DIST_SYMS];
	Symbol _maxDistanceSymbol;
public:
	SharedAncestorAndDistFT() : P1RelationFeatureType(Symbol(L"shared-ancestor-dist")) {
		for (size_t i=0; i<MAX_DISTANCE+EXTRA_DIST_SYMS; ++i)
			_distanceSymbols[i] = Symbol(boost::lexical_cast<std::wstring>(i));
		_maxDistanceSymbol = Symbol(L">="+boost::lexical_cast<std::wstring>(MAX_DISTANCE));
	}
	static const SynNode* getPronHeadNode(const SynNode* node){
		return node->getHeadPreterm();
	}
	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol );
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

		// Note: in the following code, we are saving the value of
		// m1dist to m2distSym!  This is almost certainly a bug!
		// But until we retrain the models, changing it will just 
		//hurt performance.
		Symbol m1distSym = (m1dist<MAX_DISTANCE) ? _distanceSymbols[m1dist] : _maxDistanceSymbol;
		Symbol m2distSym = (m2dist<MAX_DISTANCE) ? _distanceSymbols[m1dist] : _maxDistanceSymbol;
		
		resultArray[0] = _new DT6gramFeature(this, state.getTag(), 
			coveringNode->getTag(), m1distSym, m2distSym,
			ment1NodeP2->getHeadWord(), 
			ment2NodeP2->getHeadWord());
		return 1;
								 

	}

};

#endif
