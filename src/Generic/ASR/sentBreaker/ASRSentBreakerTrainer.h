// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ASR_SENT_BREAKER_TRAINER_H
#define ASR_SENT_BREAKER_TRAINER_H


#include "Generic/common/Symbol.h"
#include "Generic/ASR/sentBreaker/ASRSentBreakerFVecModel.h"

class NameClassTags;
//class ASRSentBreakerFVecModel;


class ASRSentBreakerTrainer {
public:
	ASRSentBreakerTrainer();

	void train();

private:
	void learnInstance(Symbol tag, Symbol word, Symbol word1, Symbol word2,
					   int tok_index);


	const static Symbol START_SENTENCE;
	const static Symbol CONT_SENTENCE;

	std::string _training_file;
	std::string _model_file;

	ASRSentBreakerFVecModel *_model;
	NameClassTags *_nameClassTags;
};

#endif

