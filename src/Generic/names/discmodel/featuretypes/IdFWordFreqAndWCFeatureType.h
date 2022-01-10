// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_WORD_FREQ_AND_WC_FEATURE_TYPE_H
#define IDF_WORD_FREQ_AND_WC_FEATURE_TYPE_H

#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"

class IdFWordFreqAndWCFeatureType  : public PIdFFeatureType {
private:
	static const int rare_threshold = 2;

public:

	IdFWordFreqAndWCFeatureType() : PIdFFeatureType(Symbol(L"word-freq&clusters"), InfoSource::OBSERVATION) {}

	~IdFWordFreqAndWCFeatureType(void){}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol,0);
	}

	virtual void validateRequiredParameters(){
		ParamReader::getRequiredParam("pidf_bigram_vocab_file");
	}

	virtual int extractFeatures(const DTState &state,
	DTFeature **resultArray) const {
		TokenObservation* o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		if(o->getWordCount() < rare_threshold)
				return 0;

		WordClusterClass wordClass = WordClusterClass(o->getSymbol().to_string());

		int n_results = 0;
		if (wordClass.c8() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c8());
		if (wordClass.c12() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c12());
		if (wordClass.c16() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c16());
		if (wordClass.c20() != 0)
			resultArray[n_results++] = _new DTIntFeature(this, state.getTag(), wordClass.c20());
		return n_results;
	}

};
#endif
