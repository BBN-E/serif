// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_MORPH_MODEL_H
#define XX_MORPH_MODEL_H

#include "Generic/common/Symbol.h"

class GenericMorphModel : public MorphModel {
private:
	friend class GenericMorphModelFactory;

public:

	~GenericMorphModel() {}

	virtual Symbol getTrainingWordFeatures(Symbol word) { return Symbol(L"NULL"); };
	virtual Symbol getTrainingReducedWordFeatures(Symbol word) { return Symbol(L"NULL"); };

private:
	GenericMorphModel() {}
	GenericMorphModel(const char* model_prefix) {};
};

class GenericMorphModelFactory: public MorphModel::Factory {
	virtual MorphModel *build(const char* model_prefix) { return _new GenericMorphModel(model_prefix); }
	virtual MorphModel *build() { return _new GenericMorphModel(); }
};



#endif
