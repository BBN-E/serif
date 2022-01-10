// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_MORPH_MODEL_H
#define AR_MORPH_MODEL_H

#include "Generic/common/Symbol.h"
#include "Generic/morphSelection/MorphModel.h"

class Lexicon;
class BWRuleDictionary;
class ParseSeeder;

class ArabicMorphModel : public MorphModel {
private:
	friend class ArabicMorphModelFactory;


public:
		
	~ArabicMorphModel();
		
		
	virtual Symbol getTrainingWordFeatures(Symbol word);
	virtual Symbol getTrainingReducedWordFeatures(Symbol word);

protected:
	ArabicMorphModel();
	ArabicMorphModel(const char* model_prefix);


private:

	Lexicon *lex;
	BWRuleDictionary *rules;

	//for getting unknown word features
	ParseSeeder *parseSeeder;
};

class ArabicMorphModelFactory: public MorphModel::Factory {
	virtual MorphModel *build(const char* model_prefix) { return _new ArabicMorphModel(model_prefix); }
	virtual MorphModel *build() { return _new ArabicMorphModel(); }
};

#endif
