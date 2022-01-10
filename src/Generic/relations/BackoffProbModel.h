// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BACKOFFPROBMODEL_H
#define BACKOFFPROBMODEL_H
#include "Generic/common/NgramScoreTable.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/relations/RelationProbModel.h"
#include <math.h>
#ifndef INIT_TABLE_SIZE
#define INIT_TABLE_SIZE 100
#endif

template <int historySize> 
class BackoffProbModel: public RelationProbModel {
public:
	static const int LOG_OF_ZERO = -10000;

	// read in a model file
	BackoffProbModel( UTF8InputStream &stream ) : _historySize(historySize),
		_observedEvents(historySize+1, INIT_TABLE_SIZE) 
	{ 
		_nGramLambdas =_new NgramScoreTable*[_historySize-1];
		_nGramProbs =_new NgramScoreTable*[_historySize];
		_initializeFormula();

		// fill lambdas and probs tables from file
		for (int i = _historySize-2; i >=0; i--)
			_nGramLambdas[i] =_new NgramScoreTable(i+2, stream);
		for (int i = _historySize-1; i >= 0; i--)
			_nGramProbs[i] =_new NgramScoreTable(i+2, stream);
		_ngram =_new Symbol[_historySize+1];

		_uniqueMultipliers =_new float[_historySize-1];
		for (int i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = 1;

	}

	// create empty tables
	BackoffProbModel() : _historySize(historySize), 
		_observedEvents(historySize+1, INIT_TABLE_SIZE)
	{

		_nGramLambdas =_new NgramScoreTable*[_historySize>0 ? _historySize-1 : 1];
		_nGramProbs =_new NgramScoreTable*[_historySize>0 ? _historySize : 1];

		for (int i = _historySize-2; i >=0; i--)
			_nGramLambdas[i] =_new NgramScoreTable(i+2, INIT_TABLE_SIZE);
		for (int i = _historySize-1; i >= 0; i--) 
			_nGramProbs[i] =_new NgramScoreTable(i+2, INIT_TABLE_SIZE);
		_ngram =_new Symbol[_historySize+1];

		_uniqueMultipliers =_new float[_historySize-1];
		for (int i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = 1;

	}

	~BackoffProbModel() {
		for (int i = _historySize-2; i >=0; i--)
			delete _nGramLambdas[i];
		for (int i = _historySize-1; i >= 0; i--) 
			delete _nGramProbs[i];
		delete[] _ngram;
		delete[] _uniqueMultipliers;
		delete[] _nGramLambdas;
		delete[] _nGramProbs;

	}

	void setUniqueMultipliers (float *uMults) {
		for (int i = 0; i < (_historySize-1); i++)
			_uniqueMultipliers[i] = uMults[i];
	}

	// query a trained model for log probability
	double getProbability( Symbol *features ) const { 
		// WARNING: _ngram may be filled with seemingly normal junk

		// construct the ngram
		int i = 0;
		for (i = 0; i < _historySize+1; i++) {
			_ngram[i] = features[i];
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
		// TODO: is it benficial to call destructors? if so, make sure it's ok
		//   delete(ngram);
		delete[] probs;
		delete[] lambdas;
		if (probTotal == 0)
			return LOG_OF_ZERO;
		return log(probTotal);

	}

	double getLambdaForFullHistory(Symbol *features) {
		// construct the ngram
		for (int i = 0; i < _historySize+1; i++) {
			_ngram[i] = features[i];
		}

		return _nGramLambdas[_historySize-2]->lookup(_ngram + 1);
	}

	double getLambdaForHistoryMinusOne(Symbol *features) {
		if (_historySize < 3)
			return 0;

		// construct the ngram
		for (int i = 0; i < _historySize+1; i++) {
			_ngram[i] = features[i];
		}

		return _nGramLambdas[_historySize-3]->lookup(_ngram + 1);
	}

	// queue up data for pending derivation
	void addEvent(Symbol *symVector) {
		_observedEvents.add(symVector);
	}

	// when the local addEvent is used
	void deriveModel( ) {
		deriveModel(&_observedEvents);
	}

	// fill in tables, following probDeriver of old
	void deriveModel(NgramScoreTable *rawData) {
		// from the constructor

		// flat (no history) case
		if (_historySize < 1) {
			// do nothing
			return;
		}
		NgramScoreTable**  histories          =_new NgramScoreTable*[_historySize];
		NgramScoreTable**  transitions        =_new NgramScoreTable*[_historySize];
		NgramScoreTable**  uniqueTransitions  =_new NgramScoreTable*[_historySize-1];
		NgramScoreTable**  storage            =_new NgramScoreTable*[_historySize-1];
		transitions[_historySize-1] = rawData; // top transitions is the raw data
		int size = transitions[_historySize-1]->get_size();
		histories[_historySize-1] =_new NgramScoreTable(_historySize, size);
		int i;
		for (i = 0; i < (_historySize-1); i++) {
			transitions[i] =_new NgramScoreTable(i+2, INIT_TABLE_SIZE);
			histories[i]   =_new NgramScoreTable(i+1, INIT_TABLE_SIZE);
		}


		// from derive_counts_and_probs
		NgramScoreTable::Table::iterator iter;
		float count;
		float history_count;
		float transition_count;
		// WARNING: _ngram may be filled with seemingly normal junk
		for (iter = transitions[_historySize-1]->get_start(); 
			iter != transitions[_historySize-1]->get_end(); ++iter) {
				count = (*iter).second;
				int j = 0;
				for (j = 0; j < (_historySize+1); j++)
					_ngram[j] = (*iter).first[j];
				for (j = 0; j < _historySize; j++) {
					histories[j]->add(_ngram + 1, count); 
					if (j < (_historySize-1))
						transitions[j]->add(_ngram, count);
				}
			}

			// first, all the unique head transitions
			// doing this from largest to smallest in case there's some 
			// dependency I can't see
			for (i = _historySize-2; i >= 0; i--) {
				uniqueTransitions[i] =_new NgramScoreTable(i+2, 
					histories[i+1]->get_size());
				for (iter = transitions[i+1]->get_start(); 
					iter != transitions[i+1]->get_end(); ++iter) {
						for (int j = 0; j < (i+2); j++)
							_ngram[j] = (*iter).first[j+1]; 
						uniqueTransitions[i]->add(_ngram, 1);
					}
			}

			// now set all the counts from head transitions into probabilities
			for (i = _historySize-1; i >= 0; i--) {
				for (iter = transitions[i]->get_start(); 
					iter != transitions[i]->get_end(); ++iter) {
						transition_count = (*iter).second;
						//        cout << "Transition[ ";
						//        cout << (*iter).first[0].to_string() << " ";
						for (int j = 0; j < (i+1); j++) {
							_ngram[j] = (*iter).first[j+1];
							//          cout << ngram[j].to_string() << " ";
						}
						history_count = histories[i]->lookup(_ngram);
						//  cout << "]: " << transition_count << " / " << history_count << " = ";
						(*iter).second = transition_count / history_count;

						//        cout << (*iter).second << "\n";

						// add it to the real part of the model
						_nGramProbs[i]->add((*iter).first, (*iter).second);
					}
			}
			// from derive_lambdas
			history_count = 0;
			float unique_count;
			// WARNING: _ngram may be filled with seemingly normal junk
			for (i = _historySize-1; i > 0; i--) { // note history[0] will be untouched
				for (iter = histories[i]->get_start(); 
					iter != histories[i]->get_end(); ++iter) {
						history_count = (*iter).second;
						//cout << "Lambda for history[ ";
						for (int j = 0; j <= i; j++) {
							_ngram[j] = (*iter).first[j];
							//cout << _ngram[j].to_string() << " ";
						}
						unique_count = uniqueTransitions[i-1]->lookup(_ngram);
						//cout << "]: " << history_count << " / (" << history_count <<" + " 
						//     << _uniqueMultipliers[i-1] << " * " << unique_count << ") = ";
						(*iter).second = history_count / 
							(history_count + _uniqueMultipliers[i-1] * unique_count);
						//cout << (*iter).second << "\n";

						// add it to the real part of the model
						_nGramLambdas[i-1]->add((*iter).first, (*iter).second);
					}
			}
	}

	void printUnderivedTables(UTF8OutputStream &out) {
		_observedEvents.print_to_open_stream(out);
	}

	void printTables(UTF8OutputStream &out)
	{
		int i = 0;
		for (i = _historySize-2; i >=0; i--)
			_nGramLambdas[i]->print_to_open_stream(out);
		for (i = _historySize-1; i >=0; i--)
			_nGramProbs[i]->print_to_open_stream(out);
	}

	int getHistorySize() { return _historySize; }

	void printFormula(UTF8OutputStream &out) const{
		out << "BackoffProbModel: " << _formula;
	}

	// returns the values used in calculating the probability.
	// NOTE: This is intended to reflect operations being performed 
	// in getProbability, but the code is not tied to it. So if that 
	// changes, so should this
	void printProbabilityElements(Symbol *features, UTF8OutputStream &out) const{
		// WARNING: _ngram may be filled with seemingly normal junk

		// construct the ngram
		for (int i = 0; i < _historySize+1; i++) {
			_ngram[i] = features[i];
		}

		// output the prob elements
		for (int i = 0; i < _historySize; i++) {
			out << "p(" << _ngram[0].to_string();
			for (int j = 1; j < i+2; j++)
				out << ";" << _ngram[j].to_string();
			out << ")=" << _nGramProbs[i]->lookup(_ngram) << " | ";
		}

		// output the lambda elements
		for (int i = 0; i < _historySize-1; i++) {
			out << "lambda(" << _ngram[1].to_string();
			for (int j = 1; j < i+2; j++)
				out << ";" << _ngram[j].to_string();
			out << ")=" << _nGramLambdas[i]->lookup(_ngram+1) << " | ";
		}
	}
private:
	// symbol array gets filled by methods that need it and 
	// changed when convenient. Don't depend on it holding state!
	Symbol* _ngram;

	const int _historySize;
	NgramScoreTable** _nGramLambdas;
	NgramScoreTable** _nGramProbs;
	NgramScoreTable _observedEvents;
	float *_uniqueMultipliers;
	string _formula;

	// create the formula template string that will be part of trace code
	// note: this is written based on what getProbability does, but
	// obviously will have to change if it does, since the two are not
	// automatically inherently linked
	void _initializeFormula () {
		// TODO: create probs and lambdas strings that have the proper f names
		string *probs =_new string[_historySize];
		string *lambdas =_new string[_historySize-1];
		int i = 0;
		for (i = 0; i < _historySize; i++) {
			char buf[50];
			sprintf(buf, "p(f0");
			for (int j = 1; j < i+2; j++)
				sprintf(buf, "%s, f%d", buf, j);
			sprintf(buf, "%s)", buf);
			probs[i] = string(buf);
		}

		// too nervous about this to do probs and lambdas in one go. 
		// It won't make much computational difference
		for (i = 0; i < _historySize-1; i++) {
			char buf[50];
			sprintf(buf, "lambda(f1");
			for (int j = 2; j < i+2; j++)
				sprintf(buf, "%s, f%d", buf, j);
			sprintf(buf, "%s)", buf);
			lambdas[i] = string(buf);
		}
		char bigbuf[200];
		sprintf(bigbuf, "%s", probs[0].c_str());
		for (i = 0; i < (_historySize-1); i++) {
			sprintf(bigbuf, "(1 - %s)*%s", lambdas[i].c_str(), string(bigbuf).c_str());
			sprintf(bigbuf, "(%s*%s)+(%s)", lambdas[i].c_str(), probs[i+1].c_str(), string(bigbuf).c_str());
		}
		_formula = string(bigbuf);

		delete [] probs;
		delete [] lambdas;
	}
};

#endif
