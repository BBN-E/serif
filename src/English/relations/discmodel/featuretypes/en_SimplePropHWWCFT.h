// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_SIMPLE_PROP_HW_WC_FT_H
#define EN_SIMPLE_PROP_HW_WC_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/theories/MentionSet.h"
#include "English/relations/en_RelationUtilities.h"

#include <boost/lexical_cast.hpp>

class EnglishSimplePropHWWCFT : public P1RelationFeatureType {
public:
	EnglishSimplePropHWWCFT() : P1RelationFeatureType(Symbol(L"simple-prop-hw-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && !link->isNested()) {
			Symbol enttype1 = o->getMention1()->getEntityType().getName();
			Symbol enttype2 = o->getMention2()->getEntityType().getName();

			int nfeatures = 0;
			wchar_t tmp[20];
			if (link->getWCArg1().c12() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg1().c12(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d", link->getWCArg1().c12());
#endif
				Symbol sym = Symbol(tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), sym, enttype2, link->getTopStemmedPred());

			}
			if (link->getWCArg1().c16() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg1().c16(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d", link->getWCArg1().c16());
#endif
				Symbol sym (tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), sym, enttype2, link->getTopStemmedPred());
			}
			if (link->getWCArg1().c20() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg1().c20(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d", link->getWCArg1().c20());
#endif
				Symbol sym(tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), sym, enttype2, link->getTopStemmedPred());
			}
			if (link->getWCArg2().c12() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg2().c12(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d", link->getWCArg2().c12());
#endif
				Symbol sym(tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), enttype1, sym, link->getTopStemmedPred());
			}
			if (link->getWCArg2().c16() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg2().c16(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d",  link->getWCArg2().c16());
#endif
				Symbol sym(tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), enttype1, sym, link->getTopStemmedPred());
			}
			if (link->getWCArg2().c20() != 0) {
#if defined(_WIN32)
				_itow(link->getWCArg2().c20(), tmp, 10);
#else
				swprintf (tmp, sizeof(tmp)/sizeof(tmp[0]), L"%d", link->getWCArg2().c20());
#endif
				Symbol sym(tmp);
				resultArray[nfeatures++] = 
					_new DTQuadgramFeature(this, state.getTag(), enttype1, sym, link->getTopStemmedPred());
			}

			return nfeatures;
		} else return 0;
	}

};

#endif
