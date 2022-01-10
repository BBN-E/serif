// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/trainers/ProbModelWriter.h"
#include "Generic/common/InternalInconsistencyException.h"

ProbModelWriter::ProbModelWriter(int N, int init_size) {
	_transitions = _new NgramScoreTable(N,init_size);
	_histories = _new NgramScoreTable(N-1, init_size);
	_uniqueTransitions = _new NgramScoreTable(N-1, init_size);
	_transitionSize = N;
}


ProbModelWriter::~ProbModelWriter() {
	delete _transitions;
	delete _histories;
	delete _uniqueTransitions;
}


void ProbModelWriter::initialize(int N, int init_size) {
	delete _transitions;
	delete _histories;
	delete _uniqueTransitions;

	_transitions = _new NgramScoreTable(N,init_size);
	_histories = _new NgramScoreTable(N-1, init_size);
	_uniqueTransitions = _new NgramScoreTable(N-1, init_size);
	_transitionSize = N;
}

void ProbModelWriter::clearModel() {
	delete _transitions;
	delete _histories;
	delete _uniqueTransitions;

	_transitions = NULL;
	_histories = NULL;
	_uniqueTransitions = NULL;
}

void ProbModelWriter::writeModel(UTF8OutputStream & stream) {
	stream << _transitions->get_size() << "\n";
	NgramScoreTable::Table::iterator iter = _transitions->get_start();
	while(iter!= _transitions->get_end()) {
		Symbol *trans = (*iter).first;
		int count = (int) (*iter).second;
		stream << "((";
		for(int i=0; i<_transitionSize-1; i++) 
			stream << trans[i].to_string() << " ";
		stream << trans[_transitionSize-1].to_string() << ") " << count << " " 
			<< (int) _histories->lookup(trans) << " " << (int) _uniqueTransitions->lookup(trans) 
			<< ")\n";
		++iter;
	}
}

void ProbModelWriter::writeLambdas(UTF8OutputStream & stream, bool inverse, float kappa) {
	stream << _histories->get_size() << "\n";
	NgramScoreTable::Table::iterator iter = _histories->get_start();
	while(iter!= _transitions->get_end()) {
		Symbol *hist = (*iter).first;
		stream << "( ";
		// same length as above, but notice no final write
		for (int i=0; i < _transitionSize-1; i++)
			stream << hist[i].to_string() << " ";
		int histnum = (int)_histories->lookup(hist);
		float uniqueNum = kappa* (int)_uniqueTransitions->lookup(hist);
		float lambda = histnum/(histnum+uniqueNum);
		if (inverse)
			lambda = 1-lambda;
		stream << lambda << " )\n";
		++iter;
	}
}




void ProbModelWriter::registerTransition(Symbol trans[]) {
	if(_transitions == NULL)
		throw InternalInconsistencyException("ProbModelWriter::registerTransition()", "No models exist!");
	_transitions->add(trans);
	_histories->add(trans);
	if((int) _transitions->lookup(trans) == 1)
		_uniqueTransitions->add(trans);
}

