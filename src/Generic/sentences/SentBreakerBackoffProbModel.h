// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SentBreakerBackoffProbModel_H
#define SentBreakerBackoffProbModel_H

#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/names/IdFWordFeatures.h"
#include <math.h>
#ifndef INIT_TABLE_SIZE
#define INIT_TABLE_SIZE 100
#endif

template <int historySize> 
class SentBreakerBackoffProbModel {
public:
	static const int LOG_OF_ZERO = -10000;

	// read in an existing model file
	SentBreakerBackoffProbModel(UTF8InputStream &stream) :
		_historySize(historySize), _observedEvents(0), 
		_nGramLambdasTraining(0), _nGramProbsTraining(0),
		_wordFeatures(IdFWordFeatures::build())
	{ 
		_nGramLambdas =_new NgramScoreTable*[_historySize-1];
		_nGramProbs =_new NgramScoreTable*[_historySize];
		int i;

		// fill lambdas and probs tables from file
		for (i = _historySize-1; i >=0; i--) {
			if (i != 0) 
				_nGramLambdas[i-1] =_new NgramScoreTable(i+1, stream);
			_nGramProbs[i] =_new NgramScoreTable(i+2, stream);
		}
		_vocabTable = _new NgramScoreTable(1, stream);	
		_ngram =_new Symbol[_historySize+1];

		_uniqueMultipliers =_new float[_historySize-1];
		for (i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = 1;

	}
	
	// create empty tables for training
	SentBreakerBackoffProbModel() : _historySize(historySize), 
		_observedEvents(0), _nGramLambdasTraining(0), _nGramProbsTraining(0),
		_wordFeatures(IdFWordFeatures::build()),
		_nGramLambdas(0), _nGramProbs(0)
	{
		_observedEvents = _new NgramScoreTable(historySize+1, INIT_TABLE_SIZE);
		_vocabTable = _new NgramScoreTable(1, INIT_TABLE_SIZE);
		_ngram =_new Symbol[_historySize+1];

		_uniqueMultipliers =_new float[_historySize-1];
		for (int i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = 1;

	}

	~SentBreakerBackoffProbModel() {
		delete _observedEvents;
		delete _vocabTable;
		delete _uniqueMultipliers;
		delete _wordFeatures;
		// should also delete _nGramLambdas & _nGramProbs, but they're never initialized in training
	}

	void setUniqueMultipliers (float *uMults) {
		for (int i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = uMults[i];
	}

	// query a trained model for log probability
	double getProbability( Symbol *features ) const { 

		// construct the ngram
		_ngram[0] = features[0];
		int i = 0;
		for (i = 1; i < _historySize+1; i++) {
			if (_vocabTable->lookup(&(features[i])) != 0)
				_ngram[i] = features[i];
			else
				_ngram[i] = _wordFeatures->features(features[i], false, false);
		}

		// find the raw probs of each full event
		double *probs =_new double[_historySize];
		for (i = 0; i < _historySize; i++) {
			probs[i] = _nGramProbs[i]->lookup(_ngram);
		}

		// find the lambdas for each history
		double *lambdas =_new double[_historySize-1];
		for (i = 0; i < (_historySize-1); i++) {
			lambdas[i] = _nGramLambdas[i]->lookup(_ngram + 1);
		}

		// iteratively construct the probability
		double probTotal = probs[0];
		for (i = 0; i < (_historySize-1); i++) {
			probTotal *= (1 - lambdas[i]);
			probTotal += (lambdas[i]*probs[i+1]);
		}

		delete(probs);
		delete(lambdas);
		if (probTotal == 0) 
			return LOG_OF_ZERO;
		return log(probTotal);
	}

	// queue up data for pending derivation
	void addEvent(Symbol *symVector) { 
		_observedEvents->add(symVector); 
		addToVocab(symVector[1], 1);
	}

	void pruneVocab(int threshold) {
                NgramScoreTable* old_table = _vocabTable;
                _vocabTable = old_table->prune(threshold);
                delete old_table;

		NgramScoreTable* new_events = _new NgramScoreTable(_historySize+1, INIT_TABLE_SIZE);

		NgramScoreTable::Table::iterator iter;
		for (iter = _observedEvents->get_start(); iter != _observedEvents->get_end(); ++iter) {
			float count = (*iter).second;
			for (int j = 0; j < (_historySize+1); j++) 
				_ngram[j] = (*iter).first[j];
			for (int k = 1; k < (_historySize+1); k++)
				if (!wordIsInVocab(_ngram[k]))
					_ngram[k] = getUnknownWordFeatures(_ngram[k]);
			new_events->add(_ngram, count);
		}

		delete _observedEvents;
		_observedEvents = new_events;
	}
		

	void deriveAndPrintModel(UTF8OutputStream &out) { deriveAndPrintModel(_observedEvents, out); }

private:

	// symbol array gets filled by methods that need it and 
	// changed when convenient. Don't depend on it holding state!
	Symbol* _ngram;

	// for training only (use instead of decode version to save memory)
	NgramScoreTable* _nGramLambdasTraining;
	NgramScoreTable* _nGramProbsTraining;

	const int _historySize;
	NgramScoreTable** _nGramLambdas;
	NgramScoreTable** _nGramProbs;
	NgramScoreTable* _vocabTable;
	NgramScoreTable* _observedEvents;
	float *_uniqueMultipliers;

	IdFWordFeatures *_wordFeatures;

	void deriveAndPrintModel(NgramScoreTable *rawData, UTF8OutputStream &out) {
		for (int i = _historySize-1; i >= 0; i--) {
			_nGramLambdasTraining = _new NgramScoreTable(i+1, INIT_TABLE_SIZE); 
			_nGramProbsTraining = _new NgramScoreTable(i+2, INIT_TABLE_SIZE);
			deriveModelByHistorySize(rawData, i);
			printTableByHistorySize(out, i);
			delete _nGramLambdasTraining;
			delete _nGramProbsTraining;
		}
		_vocabTable->print_to_open_stream(out);
	}

	void deriveModelByHistorySize(NgramScoreTable *rawData, int currSize) {
		NgramScoreTable*  histories = 0;         
		NgramScoreTable*  transitions = 0;  
		NgramScoreTable*  uniqueTransitions = 0;
		
		if (currSize == _historySize-1) {
			transitions = rawData; // top transitions is the raw data
			int size = transitions->get_size();
			histories =_new NgramScoreTable(_historySize, size);
		}
		else { 
			transitions = _new NgramScoreTable(currSize+2, INIT_TABLE_SIZE);
			histories = _new NgramScoreTable(currSize+1, INIT_TABLE_SIZE);
		}


		NgramScoreTable::Table::iterator iter;
		float count;
		float history_count;
		float transition_count;
		for (iter = rawData->get_start(); iter != rawData->get_end(); ++iter) {
			count = (*iter).second;
			for (int j = 0; j < (_historySize+1); j++)
				_ngram[j] = (*iter).first[j];
			histories->add(_ngram + 1, count);
			if (currSize < _historySize-1) 
				transitions->add(_ngram, count);
		}


		// now set all the counts from head transitions into probabilities
		for (iter = transitions->get_start(); iter != transitions->get_end(); ++iter) {
			transition_count = (*iter).second;
			for (int j = 0; j < (currSize+1); j++) {
				_ngram[j] = (*iter).first[j+1];
			}
			history_count = histories->lookup(_ngram);
			(*iter).second = transition_count / history_count;
			_nGramProbsTraining->add((*iter).first, (*iter).second);
		}

		
		history_count = 0;
		float unique_count;
		if (currSize != 0) {
			// first, all the unique head transitions
			uniqueTransitions =_new NgramScoreTable(currSize+1, histories->get_size());
			for (iter = transitions->get_start(); iter != transitions->get_end(); ++iter) {
				for (int j = 0; j < (currSize+1); j++)
					_ngram[j] = (*iter).first[j+1]; 
				uniqueTransitions->add(_ngram, 1);
			}
			// then, calculate the lambdas
			for (iter = histories->get_start(); iter != histories->get_end(); ++iter) {
				history_count = (*iter).second;
				for (int j = 0; j <= currSize; j++) {
					_ngram[j] = (*iter).first[j];
				}
				unique_count = uniqueTransitions->lookup(_ngram);
				(*iter).second = history_count / 
					(history_count + _uniqueMultipliers[currSize-1] * unique_count);
				_nGramLambdasTraining->add((*iter).first, (*iter).second);
			}
		}

		delete uniqueTransitions;
		delete histories;
		if (currSize != _historySize-1) // make sure not to delete rawData
			delete transitions;
	}

	void printTableByHistorySize(UTF8OutputStream &out, int index) {
		if (index != 0)
			_nGramLambdasTraining->print_to_open_stream(out);
		_nGramProbsTraining->print_to_open_stream(out);
	}

	void addToVocab(Symbol word, int count) {
		_vocabTable->add(&word, static_cast<float>(count));
	}

	bool wordIsInVocab(Symbol word) {
		return (_vocabTable->lookup(&word) != 0);
	}
	
	Symbol getUnknownWordFeatures(Symbol word) {
		return _wordFeatures->features(word, false, false);
	}
};

#endif
