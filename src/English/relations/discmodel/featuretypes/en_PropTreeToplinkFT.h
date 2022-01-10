// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN____PROPTREE_TOPLINK_FT_H___
#define EN____PROPTREE_TOPLINK_FT_H___

//#include "Generic/common/Symbol.h"
//#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/PropTreeLinks.h"
//#include "English/relations/en_RelationUtilities.h"


class EnglishPropTreeToplinkFT : public P1RelationFeatureType {
 public:
  EnglishPropTreeToplinkFT() : P1RelationFeatureType(Symbol(L"proptree-toplink")) {}
  
  virtual DTFeature *makeEmptyFeature() const {
    return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
				  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol);
  }
  
  virtual int extractFeatures(const DTState &state, DTFeature **resultArray) const {
    RelationObservation *o = static_cast<RelationObservation*>(state.getObservation(0));
    
	//RelationPropLink *link = o->getPropLink();
	//if ( ! link->isEmpty() ) return 0; //compute these features _only_ if there's no direct proposition connection

    const TreeNodeChain* tnChain=o->getPropTreeNodeChain();
	if ( ! tnChain ) return 0;
	int dist = (int) tnChain->getDistance();
	int topLink = tnChain->getTopLink();
	int size = tnChain->getSize();
	const TreeNodeElement* tnel=tnChain->getElement(topLink);
	if ( ! tnel ) { std::wcerr << "\nweird internal error: no toplink element!"; return 0; }
	std::wstring leftTopRole=topLink == 0 ? L"-" : tnChain->getElement(topLink-1)->_role.c_str();
	std::wstring rightTopRole=topLink == size-1 ? L"-" : tnChain->getElement(topLink+1)->_role.c_str();
	//apparently, it's better to consider the role of a mention even if it's a toplink of the connection!
	std::wstring leftImmedRole=topLink == 0 ? L"-" : tnChain->getElement(0)->_role.c_str();
	std::wstring rightImmedRole=topLink == size-1 ? L"-" : tnChain->getElement(size-1)->_role.c_str();

	std::wstring leftEtype=o->getMention1()->getEntityType().getName().to_string();
	std::wstring rightEtype=o->getMention2()->getEntityType().getName().to_string();
	std::wstring leftEStype=o->getMention1()->getEntitySubtype().getName().to_string();
	std::wstring rightEStype=o->getMention2()->getEntitySubtype().getName().to_string();

	wchar_t d1[20], d2[20];
#ifdef WIN32
	_itow(topLink,d1,10);
	_itow(dist-topLink,d2,10);
#else
	swprintf (d1, sizeof(d1)/sizeof(d1[0]), L"%d", topLink);
	swprintf (d2, sizeof(d2)/sizeof(d2[0]), L"%d", dist-topLink);
#endif

	int k=0;
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftEtype.c_str(), d1, rightTopRole.c_str());
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftTopRole.c_str(), d2, rightEtype.c_str());

	//sequence of roles to reach the mentions from the toplink
	std::wstring ch1=L"LRSEQ";
	if ( topLink >= 4 ) ch1 += L"+FARAWAY";
	else {
		for ( int i=0; i < topLink; i++ ) {
			ch1 += L"+";
			ch1 += tnChain->getElement(i)->_role;
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), ch1.c_str(), d2, rightImmedRole.c_str());
	std::wstring ch2=L"RRSEQ";
	if ( dist-topLink >= 4 ) ch2 += L"+FARAWAY";
	else {
		for ( int i=size-1; i > topLink; i-- ) {
			ch2 += L"+";
			ch2 += tnChain->getElement(i)->_role;
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), ch2.c_str(), d1, leftImmedRole.c_str() );

	/*
	//number of alternatives that we select our mentions from (two one-sided measures)
	int alt1=1,alt2=1;
	for ( int i=1; i <= topLink; i++ ) alt1 *= tnChain->getElement(i)->_arity - (i==topLink?1:0);
	for ( int i=size-1; i >= topLink; i-- ) alt2 *= tnChain->getElement(i)->_arity - (i==topLink?1:0);
	std::wstring a1=L"*MANY*", a2=L"*MANY*";
	if ( alt1 <= 1 ) a1 = L"[=1";
	else if ( alt1 <= 4 ) a1 = L"[=4";
	else if ( alt1 <= 8 ) a1 = L"[=8";
	else if ( alt1 <= 12 ) a1 = L"[=12";
	if ( alt2 <= 1 ) a2 = L"[=1";
	else if ( alt2 <= 4 ) a2 = L"[=4";
	else if ( alt2 <= 8 ) a2 = L"[=8";
	else if ( alt2 <= 12 ) a2 = L"[=12";

	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftImmedRole.c_str(), a1.c_str(), rightEtype.c_str());
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftEtype.c_str(), a2.c_str(), rightImmedRole.c_str());
	*/

	//sequence of entity types to reach the mentions from the toplink
	std::wstring es1=L"LESEQ";
	if ( topLink >= 4 ) es1 += L"*FARAWAY";
	else {
		for ( int i=0; i <= topLink; i++ ) {
			es1 += L"*";
			es1 += tnChain->getElement(i)->_etype;
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), es1.c_str(), leftImmedRole.c_str(), rightTopRole.c_str());
	std::wstring es2=L"RESEQ";
	if ( dist-topLink >= 4 ) es2 += L"*FARAWAY";
	else {
		for ( int i=size-1; i >= topLink; i-- ) {
			es2 += L"*";
			es2 += tnChain->getElement(i)->_etype;
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), es2.c_str(), rightImmedRole.c_str(), leftTopRole.c_str());


	/*/sequence of entity SUBtypes to reach the mentions from the toplink
	std::wstring ss1=L"LSSEQ";
	if ( topLink >= 4 ) ss1 += L"*FARAWAY";
	else {
		for ( int i=0; i <= topLink; i++ ) {
			const Mention* m=tnChain->getElement(i)->_men;
			ss1 += L"*";
			ss1 += m ? m->getEntitySubtype().getName().to_string() : L"NONE";
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), ss1.c_str(), leftImmedRole.c_str(), rightTopRole.c_str());
	std::wstring ss2=L"RSSEQ";
	if ( dist-topLink >= 4 ) ss2 += L"*FARAWAY";
	else {
		for ( int i=size-1; i >= topLink; i-- ) {
			const Mention* m=tnChain->getElement(i)->_men;
			ss2 += L"*";
			ss2 += m ? m->getEntitySubtype().getName().to_string() : L"NONE";
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), ss2.c_str(), rightImmedRole.c_str(), leftTopRole.c_str());
	*/

	/*
	//sequence of mention types (on both sides)
	std::wstring mt1=L"LMTS";
	for ( int i=0; i < topLink; i++ ) {
		if ( ! tnChain->getElement(i)->_men ) {
			mt1 += L"-NONE";
			continue;
		}
		Mention::Type mtype=tnChain->getElement(i)->_men->getMentionType();
		switch ( mtype ) {
			case Mention::NAME: mt1 += L"-NAME"; break;
			case Mention::PRON: mt1 += L"-PRON"; break;
			case Mention::DESC: mt1 += L"-DESC"; break;
			default: mt1 += L"-ELSE"; break;
		}
	}
	std::wstring mt2=L"RMTS";
	for ( int i=size-1; i > topLink; i-- ) {
		if ( ! tnChain->getElement(i)->_men ) {
			mt2 += L"-NONE";
			continue;
		}
		Mention::Type mtype=tnChain->getElement(i)->_men->getMentionType();
		switch ( mtype ) {
			case Mention::NAME: mt2 += L"-NAME"; break;
			case Mention::PRON: mt2 += L"-PRON"; break;
			case Mention::DESC: mt2 += L"-DESC"; break;
			default: mt2 += L"-ELSE"; break;
		}
	}
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), mt1.c_str(), tnChain->getElement(topLink)->_text.c_str(), mt2.c_str());
	*/

	//resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), (leftEtype + L"-L").c_str(), rightTopRole.c_str(), rightEtype.c_str());
	//resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), (rightEtype + L"-R").c_str(), leftTopRole.c_str(), leftEtype.c_str());

	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftEStype.c_str(), d1, rightTopRole.c_str());
	resultArray[k++] = _new DTQuadgramFeature(this, state.getTag(), leftTopRole.c_str(), d2, rightEStype.c_str());

	return k;
  }
  
};

#endif
