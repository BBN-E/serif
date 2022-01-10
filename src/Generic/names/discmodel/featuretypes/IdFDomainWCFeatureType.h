// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_DOMAIN_WC_FEATURE_TYPE_H
#define D_T_IDF_DOMAIN_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"

class IdFDomainWCFeatureType : public PIdFFeatureType {
public:
	IdFDomainWCFeatureType() : PIdFFeatureType(Symbol(L"domain-wc"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
		DTFeature **resultArray) const
	{
		int n_results = 0;

		// The token itself
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		WordClusterClass wordClass = WordClusterClass(o->getSymbol(), false, true);
		if (wordClass.c8() != 0)
			resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"wc8")), state.getTag(), wordClass.c8());
		if (wordClass.c12() != 0)
			resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"wc12")), state.getTag(), wordClass.c12());
		if (wordClass.c16() != 0)
			resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"wc16")), state.getTag(), wordClass.c16());
		if (wordClass.c20() != 0)
			resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"wc20")), state.getTag(), wordClass.c20());

		// Next token
		if (state.getIndex()+1 < state.getNObservations()) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+1));
			wordClass = WordClusterClass(o->getSymbol(), false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next-wc8")), state.getTag(), wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next-wc12")), state.getTag(), wordClass.c12());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next-wc16")), state.getTag(), wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next-wc20")), state.getTag(), wordClass.c20());
		}

		// Next next token
		if (state.getIndex()+2 < state.getNObservations()) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+2));
			wordClass = WordClusterClass(o->getSymbol(), false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next2-wc8")), state.getTag(), wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next2-wc12")), state.getTag(), wordClass.c12());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next2-wc16")), state.getTag(), wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"next2-wc20")), state.getTag(), wordClass.c20());
		}

		// Previous token
		if (state.getIndex()-1 >= 0) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-1));
			wordClass = WordClusterClass(o->getSymbol(), false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev-wc8")), state.getTag(), wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev-wc12")), state.getTag(), wordClass.c12());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev-wc16")), state.getTag(), wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev-wc20")), state.getTag(), wordClass.c20());
		}

		// Previous previous token
		if (state.getIndex()-2 >= 0) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-2));
			wordClass = WordClusterClass(o->getSymbol(), false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev2-wc8")), state.getTag(), wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev2-wc12")), state.getTag(), wordClass.c12());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev2-wc16")), state.getTag(), wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTIntFeature(DTFeatureType::getFeatureType(PIdFFeatureType::modeltype, Symbol(L"prev2-wc20")), state.getTag(), wordClass.c20());
		}

		return n_results;
	}


};

#endif
