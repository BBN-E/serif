// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_WORD_WC_FT_H
#define HEAD_WORD_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/relations/xx_RelationUtilities.h"
#include "Generic/wordClustering/WordClusterClass.h"

#include <boost/lexical_cast.hpp>

class HeadWordWCFT : public P1RelationFeatureType {
public:
	HeadWordWCFT() : P1RelationFeatureType(Symbol(L"head-word-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));
		
		Symbol head1 = o->getMention1()->getNode()->getHeadWord();
		Symbol head2 = o->getMention2()->getNode()->getHeadWord();
		Symbol enttype1 = o->getMention1()->getEntityType().getName();
		Symbol enttype2 = o->getMention2()->getEntityType().getName();

		WordClusterClass arg1WC = WordClusterClass::nullCluster();
		WordClusterClass arg2WC = WordClusterClass::nullCluster();
		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			arg1WC = link->getWCArg1();
			arg2WC = link->getWCArg2();
		}
		else {
			arg1WC = WordClusterClass(head1);
			arg2WC = WordClusterClass(head2);
		}

		int nfeatures = 0;
#if defined(_WIN32)
		wchar_t tmp[20];
		if (arg1WC.c12() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(_itow(arg1WC.c12(), tmp, 10)), enttype2);
		if (arg1WC.c16() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(_itow(arg1WC.c16(), tmp, 10)), enttype2);
		if (arg1WC.c20() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(_itow(arg1WC.c20(), tmp, 10)), enttype2);
		
		if (arg2WC.c12() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(_itow(arg2WC.c12(), tmp, 10)));
		if (arg2WC.c16() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(_itow(arg2WC.c16(), tmp, 10)));
		if (arg2WC.c20() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(_itow(arg2WC.c20(), tmp, 10)));
#else
		if (arg1WC.c12() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(boost::lexical_cast<std::wstring>(arg1WC.c12()).c_str()), enttype2);
		if (arg1WC.c16() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(boost::lexical_cast<std::wstring>(arg1WC.c16()).c_str()), enttype2);
		if (arg1WC.c20() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				Symbol(boost::lexical_cast<std::wstring>(arg1WC.c20()).c_str()), enttype2);
		
		if (arg2WC.c12() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(boost::lexical_cast<std::wstring>(arg2WC.c12()).c_str()));
		if (arg2WC.c16() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(boost::lexical_cast<std::wstring>(arg2WC.c16()).c_str()));
		if (arg2WC.c20() != 0) 
			resultArray[nfeatures++] = _new DTTrigramFeature(this, state.getTag(),
				enttype1, Symbol(boost::lexical_cast<std::wstring>(arg2WC.c20()).c_str()));
#endif
		
		return nfeatures;	
	}
};

#endif
