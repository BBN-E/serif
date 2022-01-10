// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_SENT_BREAKER_TRAINER_H
#define STAT_SENT_BREAKER_TRAINER_H


#include "Generic/common/Symbol.h"
#include "Generic/names/IdFWordFeatures.h"
#include "Generic/sentences/StatSentBreakerFVecModel.h"


class NameClassTags;


class StatSentBreakerTrainer {
public:
	StatSentBreakerTrainer(int mode);

	void train();
	void devtest();

	enum { TRAIN, DEVTEST };
private:
	void learnInstance(Symbol tag, Symbol word, Symbol word1, Symbol word2,
					   int tok_index);
	double getSTScore(Symbol word, Symbol word1, Symbol word2);

	const static Symbol START_SENTENCE;
	const static Symbol CONT_SENTENCE;

	std::string _training_file;
	std::string _model_file;
	std::string _devtest_file;

	StatSentBreakerFVecModel *_model;
	NameClassTags *_nameClassTags;

	int _pruning_threshold;
	int _mode;
};

#endif

