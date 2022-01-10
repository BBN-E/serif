// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_SECONDARY_WC_FEATURE_TYPE_H
#define D_T_IDF_SECONDARY_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"

class IdFSecondaryWCFeatureType : public PIdFFeatureType {
public:
 IdFSecondaryWCFeatureType() : PIdFFeatureType(Symbol(L"secondary-wc"), InfoSource::OBSERVATION),
		WC8(L"wc8"), WC12(L"wc12"), WC16(L"wc16"), WC20(L"wc20"), NEXT_WC8(L"next_wc8"), 
		NEXT_WC12(L"next_wc12"), NEXT_WC16(L"next_wc16"), NEXT_WC20(L"next_wc20"), 
		NEXT2_WC8(L"next2_wc8"), NEXT2_WC12(L"next2_wc12"), NEXT2_WC16(L"next2_wc16"), 
		NEXT2_WC20(L"next2_wc20"), PREV_WC8(L"prev_wc8"), PREV_WC12(L"prev_wc12"), 
		PREV_WC16(L"prev_wc16"), PREV_WC20(L"prev_wc20"), PREV2_WC8(L"prev2_wc8"), 
		PREV2_WC12(L"prev2_wc12"), PREV2_WC16(L"prev2_wc16"), PREV2_WC20(L"prev2_wc20")
		{}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, -1);
	}


	virtual int extractFeatures(const DTState &state,
		DTFeature **resultArray) const
	{
		int n_results = 0;

		// The token itself
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		WordClusterClass wordClass = WordClusterClass(o->getSymbol(), false, false, true);
		if (wordClass.c8() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), WC8, wordClass.c8());
		if (wordClass.c12() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), WC12, wordClass.c12());
		if (wordClass.c16() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), WC16, wordClass.c16());
		if (wordClass.c20() != 0)
			resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), WC20, wordClass.c20());

		// Next token
		if (state.getIndex()+1 < state.getNObservations()) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+1));
			wordClass = WordClusterClass(o->getSymbol(), false, false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT_WC8, wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT_WC16, wordClass.c16());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT_WC16, wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT_WC20, wordClass.c20());
		}

		// Next next token
		if (state.getIndex()+2 < state.getNObservations()) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()+2));
			wordClass = WordClusterClass(o->getSymbol(), false, false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT2_WC8, wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT2_WC12, wordClass.c8());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT2_WC16, wordClass.c8());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), NEXT2_WC20, wordClass.c8());
		}

		// Previous token
		if (state.getIndex()-1 >= 0) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-1));
			wordClass = WordClusterClass(o->getSymbol(), false, false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV_WC8, wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV_WC16, wordClass.c16());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV_WC16, wordClass.c16());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV_WC20, wordClass.c20());
		}

		// Previous previous token
		if (state.getIndex()-2 >= 0) {
			o = static_cast<TokenObservation*>(state.getObservation(state.getIndex()-2));
			wordClass = WordClusterClass(o->getSymbol(), false, false, true);
			if (wordClass.c8() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV2_WC8, wordClass.c8());
			if (wordClass.c12() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV2_WC12, wordClass.c8());
			if (wordClass.c16() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV2_WC16, wordClass.c8());
			if (wordClass.c20() != 0)
				resultArray[n_results++] = _new DTBigramIntFeature(this, state.getTag(), PREV2_WC20, wordClass.c8());
		}

		return n_results;
	}

 private:
	const Symbol WC8;
	const Symbol WC12;
	const Symbol WC16;
	const Symbol WC20;
	const Symbol NEXT_WC8;
	const Symbol NEXT_WC12;
	const Symbol NEXT_WC16;
	const Symbol NEXT_WC20;
	const Symbol NEXT2_WC8;
	const Symbol NEXT2_WC12;
	const Symbol NEXT2_WC16;
	const Symbol NEXT2_WC20;
	const Symbol PREV_WC8;
	const Symbol PREV_WC12;
	const Symbol PREV_WC16;
	const Symbol PREV_WC20;
	const Symbol PREV2_WC8;
	const Symbol PREV2_WC12;
	const Symbol PREV2_WC16;
	const Symbol PREV2_WC20;

};

#endif
