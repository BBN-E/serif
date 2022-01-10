// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KR_MORPH_MODEL_H
#define KR_MORPH_MODEL_H

#include "common/Symbol.h"
#include "morphSelection/KoreanMorphModel.h"

class KoreanMorphologicalAnalyzer;

class KoreanMorphModel : public MorphModel {

private:
	friend class KoreanMorphModelFactory;

public:
	~KoreanMorphModel() {};

	virtual Symbol getTrainingWordFeatures(Symbol word);
	virtual Symbol getTrainingReducedWordFeatures(Symbol word);

private:
	KoreanMorphModel() {};
	KoreanMorphModel(const char* model_prefix, KoreanMorphologicalAnalyzer *morphAnalyzer = 0);
	
};


class KoreanMorphModelFactory: public MorphModel::Factory {
	virtual MorphModel *build(const char* model_prefix) { return _new KoreanMorphModel(model_prefix); }
	virtual MorphModel *build() { return _new KoreanMorphModel(); }
};

#endif
