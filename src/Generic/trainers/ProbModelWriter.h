// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROBMODELWRITER_H
#define PROBMODELWRITER_H

//#include "Generic/common/ProbModel.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/NgramScoreTable.h"

class ProbModelWriter {
public:
	ProbModelWriter(int N, int init_size);
	~ProbModelWriter();
	void initialize(int N, int init_size);
	void clearModel();
	void writeModel(UTF8OutputStream & stream);
	// witten-bell backoff
	// used for nameLinkerTrainer
	void writeLambdas(UTF8OutputStream & stream, bool inverse=false, float kappa=1.0);
	void registerTransition(Symbol trans[]);
private:
	NgramScoreTable *_transitions, *_histories, *_uniqueTransitions;
	int _transitionSize;
};

#endif
