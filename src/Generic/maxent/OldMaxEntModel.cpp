// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/maxent/OldMaxEntEvent.h"
#include "Generic/maxent/OldMaxEntEventSet.h"
#include "Generic/maxent/OldMaxEntModel.h"
#include <math.h>
#include <time.h>
#include <boost/scoped_ptr.hpp>

#ifndef INIT_TABLE_SIZE
#define INIT_TABLE_SIZE 100
#endif
#ifndef _LOG_OF_ZERO
#define _LOG_OF_ZERO -10000
#endif
#ifndef LOG
#define LOG(x) (x==0.0 ? _LOG_OF_ZERO : log(x))
#endif
#ifndef MAX_FEATURES_TO_ADD
#define MAX_FEATURES_TO_ADD 10
#endif

DebugStream OldMaxEntModel::_debug;

OldMaxEntModel::OldMaxEntModel(const char *prefix) : 
	_alphaTable(0), _observedEvents(0), _heldOutEvents(0), _outcomes(0),
	_trainMode(0), _stopCriterion(0), _percentHeldOut(0), _variance(0), _n_features_to_add(0)
{ 

	if (!_debug.isActive()) 
		_debug.init(Symbol(L"maxent_debug"));

	UTF8Token token;
	char buffer[500];

	sprintf(buffer, "%s.maxent", prefix);
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(buffer);
	stream >> _constantC;
	stream >> _correctionAlpha;
	stream >> _n_outcomes;
	_outcomes = _new Symbol[_n_outcomes];
	for (int i = 0; i < _n_outcomes; i++) {
		stream >> token;
		_outcomes[i] = token.symValue();
	}
	_alphaTable = _new OldMaxEntFeatureTable(stream);
	stream.close();
	
} 

OldMaxEntModel::OldMaxEntModel(UTF8InputStream &stream) : 
	_alphaTable(0), _observedEvents(0), _heldOutEvents(0), _outcomes(0),
	_trainMode(0), _stopCriterion(0), _percentHeldOut(0), _variance(0), _n_features_to_add(0)
{ 
	if (!_debug.isActive()) 
		_debug.init(Symbol(L"maxent_debug"));

	UTF8Token token;

	stream >> _constantC;
	stream >> _correctionAlpha;
	stream >> _n_outcomes;
	_outcomes = _new Symbol[_n_outcomes];
	for (int i = 0; i < _n_outcomes; i++) {
		stream >> token;
		_outcomes[i] = token.symValue();
	}
	_alphaTable = _new OldMaxEntFeatureTable(stream);
} 

OldMaxEntModel::OldMaxEntModel(int n_outcomes, Symbol *outcomes, int mode, int stop_criterion, 
						   int percent_held_out, double variance, int n_features_to_add) : 
	_alphaTable(0), _observedEvents(0), _heldOutEvents(0), 
	_outcomes(0), _trainMode(mode), _stopCriterion(stop_criterion),
	_percentHeldOut(percent_held_out), _variance(variance),
	_n_features_to_add(n_features_to_add)
{ 
	if (!_debug.isActive()) 
		_debug.init(Symbol(L"maxent_debug"));

	_alphaTable = _new OldMaxEntFeatureTable(INIT_TABLE_SIZE);
	_observedEvents = _new OldMaxEntEventSet(outcomes, n_outcomes);
	_heldOutEvents  = _new OldMaxEntEventSet(outcomes, n_outcomes);
	_n_events_added = 0;
	_n_outcomes = n_outcomes;
	_outcomes = _new Symbol[_n_outcomes];
	for (int i = 0; i < _n_outcomes; i++)
		_outcomes[i] = outcomes[i];

	// Automatically add "prior" features to the feature set to ensure that
	// every possible event has at least one active feature (requirement for GIS)
	//_observedEvents->addPriorFeatures();
}

OldMaxEntModel::~OldMaxEntModel() {
	delete _alphaTable;
	delete _observedEvents;
	delete _heldOutEvents;
	delete [] _outcomes;
}

// Note: this is inefficent if you're going to compare probabilities
// for all outcomes since each call recalculates the probability
// for every outcome to get the context probability for normalization.
double OldMaxEntModel::getProbability(OldMaxEntEvent *event) const {
	
	Symbol actual_outcome = event->getOutcome();	

	double context_prob = 0;
	double score;
	for (int i = 0; i < _n_outcomes; i++) {
		event->setOutcome(_outcomes[i]);
		score = getScore(event);
		context_prob += exp(score);
		
	}

	event->setOutcome(actual_outcome);
	score = getScore(event);
	return exp(score) / context_prob;
}


double OldMaxEntModel::getScore(OldMaxEntEvent *event) const {
	int n_features_used = 0;
	double score = 0;
	Symbol outcome = event->getOutcome();
	for (int i = 0; i < event->getNContextPredicates(); i++) {
		double val = _alphaTable->lookup(outcome, event->getContextPredicate(i));
		if (val == 0) 
			continue;
		score += val;
		n_features_used++;
		if (n_features_used >= _constantC)
			break;
	}
	if (n_features_used < _constantC)
		score += (_constantC - n_features_used) * _correctionAlpha;
	return score;
}

double OldMaxEntModel::getScoreAndDebug(OldMaxEntEvent *event, DebugStream& debug) const {
	int n_features_used = 0;
	double score = 0;
	Symbol outcome = event->getOutcome();
	debug << "Calculating score for outcome: " << outcome.to_debug_string() << "\n";
	for (int i = 0; i < event->getNContextPredicates(); i++) {
		double val = _alphaTable->lookup(outcome, event->getContextPredicate(i));
		if (val == 0) 
			continue;
		score += val;
		debug << event->getContextPredicate(i).to_string() << " " << val << "\n";
		n_features_used++;
		if (n_features_used >= _constantC)
			break;
	}
	if (n_features_used < _constantC) {
		score += (_constantC - n_features_used) * _correctionAlpha;
		debug << "Correction Alpha Value: " << (_constantC - n_features_used) * _correctionAlpha << "\n";
	}
	debug << "FINAL SCORE: " << score << "\n\n";
	return score;
}

double OldMaxEntModel::getScoreAndDebug(OldMaxEntEvent *event, UTF8OutputStream& debug) const {
	int n_features_used = 0;
	double score = 0;
	Symbol outcome = event->getOutcome();
	debug << "Calculating score for outcome: " << outcome.to_debug_string() << "\n";
	for (int i = 0; i < event->getNContextPredicates(); i++) {
		double val = _alphaTable->lookup(outcome, event->getContextPredicate(i));
		if (val == 0) 
			continue;
		score += val;
		debug << event->getContextPredicate(i).to_string() << " " << val << "\n";
		n_features_used++;
		if (n_features_used >= _constantC)
			break;
	}
	if (n_features_used < _constantC) {
		score += (_constantC - n_features_used) * _correctionAlpha;
		debug << "Correction Alpha Value: " << (_constantC - n_features_used) * _correctionAlpha << "\n";
	}
	debug << "FINAL SCORE: " << score << "\n\n";
	return score;
}

void OldMaxEntModel::addEvent(OldMaxEntEvent *event, int count) { 
	if (_percentHeldOut > 0) {
		int n = 100 / _percentHeldOut;
		if (count == 1 && ((_n_events_added) % n == 0)) {
			_heldOutEvents->addEvent(event, 1, _debug);
		} else if ((_n_events_added % n) == 0) {
			_heldOutEvents->addEvent(event,
				static_cast<int>(ceil((double)count / n)), _debug);
			_observedEvents->addEvent(event,
				static_cast<int>(count - ceil((double)count / n)), _debug);
		} else if ((_n_events_added % n) + count > n) {
			_heldOutEvents->addEvent(event,
				static_cast<int>(floor((((_n_events_added % n) + (double)count) / n))), _debug);
			_observedEvents->addEvent(event,
				static_cast<int>(count - floor((((_n_events_added % n) + (double)count) / n))), _debug);
		}
		else {
			_observedEvents->addEvent(event, count, _debug); 
		}
	}
	else {
		_observedEvents->addEvent(event, count, _debug); 
	}
	_n_events_added += count;
}

void OldMaxEntModel::deriveModel(int pruning_threshold, double threshold) { 

	time_t start_time;
	time(&start_time);
	
	
	/*if (_debug.isActive()) {
		_debug << "***************************************************************************\n\n";
		_debug << "Before Pruning:\n\n";
		_debug << "***************************************************************************\n\n";
		_observedEvents->dump(_debug);
		_debug << "***************************************************************************\n\n";
		_debug << "***************************************************************************\n\n";
		_debug << "Held Out Events:\n\n";
		_debug << "***************************************************************************\n\n";
		_heldOutEvents->dump(_debug);
		_debug << "***************************************************************************\n\n";
	}*/
	
	_observedEvents->prune(pruning_threshold, _debug);
	
	/*if (_debug.isActive() && pruning_threshold > 1) {
		_debug << "***************************************************************************\n\n";
		_debug << "After Pruning:\n\n";
		_debug << "***************************************************************************\n\n";
		_observedEvents->dump(_debug);
		_debug << "***************************************************************************\n\n";
		_debug << "Iterative Scaling:\n\n";
		_debug << "***************************************************************************\n\n";
	}*/


	/*OldMaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
	while (fIter != _observedEvents->featureIterEnd()) {
			Symbol outcome = (*fIter).first->getOutcome();
			SymbolSet predicate = (*fIter).first->getPredicate();
			int featureID = _observedEvents->getFeatureID(outcome, predicate);
			_debug << "Feature [" << featureID << "] " << predicate.to_string() << " " << outcome.to_string() << "\n";
			++fIter;
	}
	_debug << "\n";*/

	_constantC = _observedEvents->getMaxContextFeatures();
	_inverseC = (_constantC == 0) ? 0 : 1/(double)_constantC;

	// Initialize data structures for training
    _observedContextProbs = _new double[_observedEvents->getNContexts()];	
	_observedEventProbs = _new double*[_observedEvents->getNContexts()];
	_predictedProbs = _new double*[_observedEvents->getNContexts()];
	_lastPredictedProbs = _new double*[_observedEvents->getNContexts()];
	_contexts = _new EventContext*[_observedEvents->getNContexts()];
	_featureIDs = _new int**[_observedEvents->getNContexts()];
	_predicateCounts = _new int*[_observedEvents->getNContexts()];

	_logAlphas = _new double[_observedEvents->getNFeatures() + 1];
	_observedFeatureProbs = _new double[_observedEvents->getNFeatures() + 1];
	_predictedExpectation = _new double[_observedEvents->getNFeatures() + 1];
	_featureOutcomes = _new int[_observedEvents->getNFeatures() + 1];
	_activeFeature = _new int[_observedEvents->getNFeatures() + 1];

	// Initialize _featureOutcomes and _activeFeatures
	for (int f = 0; f < _observedEvents->getNFeatures() + 1; f++) {
		_featureOutcomes[f] = -1;
		if (_trainMode == IIS_FEATURE_SELECTION) 
			_activeFeature[f] = 0;
		else
			_activeFeature[f] = 1;
	}
	

	// Initialize last predicted probs for each event type
	// store contexts, events, and feature ids, etc. in arrays for faster lookup 
	ContextIter cIter = _observedEvents->contextIter();
	for (int i = 0; i < _observedEvents->getNContexts(); i++) {
		EventContext *context = cIter.findNext();
		_contexts[i] = context;
		_featureIDs[i] = _new int*[_n_outcomes];
		_predicateCounts[i] = _new int[_n_outcomes];
		_observedEventProbs[i] = _new double[_n_outcomes];
		_predictedProbs[i] = _new double[_n_outcomes + 1];
		_lastPredictedProbs[i] = _new double[_n_outcomes + 1];
		for (int j = 0; j < _n_outcomes; j++) {
			_lastPredictedProbs[i][j] = 0;
			_featureIDs[i][j] = _new int[context->getNPredicates()];
			_predicateCounts[i][j] = 0;
			for (int k = 0; k < context->getNPredicates(); k++) {
				int id = _observedEvents->getFeatureID(_outcomes[j], context->getPredicate(k));
				_featureIDs[i][j][k] = id;
				if (id != -1) {
					_featureOutcomes[id] = j;
					if (_activeFeature[id])
						_predicateCounts[i][j]++;
				}
			}	
		}
		_lastPredictedProbs[i][_n_outcomes] = 0;
	}
	

	// Collect information about held out events
	_heldOutObservedEventProbs = _new double*[_heldOutEvents->getNContexts()];
	_heldOutPredictedProbs = _new double*[_heldOutEvents->getNContexts()];
	_heldOutContexts = _new EventContext*[_heldOutEvents->getNContexts()];
	_heldOutFeatureIDs = _new int**[_heldOutEvents->getNContexts()];
	_heldOutPredicateCounts = _new int*[_observedEvents->getNContexts()];

	ContextIter hIter = _heldOutEvents->contextIter();
	for (int a = 0; a < _heldOutEvents->getNContexts(); a++) {
		EventContext *context = hIter.findNext();
		_heldOutContexts[a] = context;
		_heldOutFeatureIDs[a] = _new int*[_n_outcomes];
		_heldOutPredicateCounts[a] = _new int[_n_outcomes];
		_heldOutObservedEventProbs[a] = _new double[_n_outcomes];
		_heldOutPredictedProbs[a] = _new double[_n_outcomes + 1];
		for (int b = 0; b < _n_outcomes; b++) {
			_heldOutFeatureIDs[a][b] = _new int[context->getNPredicates()];
			_heldOutPredicateCounts[a][b] = 0;
			for (int c = 0; c < context->getNPredicates(); c++) {
				int id = _observedEvents->getFeatureID(_outcomes[b], context->getPredicate(c));
				_heldOutFeatureIDs[a][b][c] = id;
				if (id != -1 && _activeFeature[id]) {
					_heldOutPredicateCounts[a][b]++;
				}
			}	
		}
	}

	// Initialize observed probs and alphas
	calculateObservedProbs();
	initializeAlphas();

   /*
	*				Feature Iteration section. 
	* For each iteration, calculate feature that will result in
	* max gain, then add to model and retrain.
	*/
	bool features_converged = false;
	int features_iter = 0;
	double last_held_out_likelihood = -1000;
	double last_ten_diffs[10];
	for (int t = 0; t < 10; t++)
		last_ten_diffs[t] = 0;
	int features_to_add[MAX_FEATURES_TO_ADD];

	while (!features_converged) {
		features_converged = true;
		
		// Select the feature(s) with max potential gain and add it/them to model
		if (_trainMode == IIS_FEATURE_SELECTION) {
			findMaxFeatureGain(features_iter, threshold, features_converged, features_to_add, _n_features_to_add);
			cout << "iteration #" << features_iter << " added feature numbers ";
			for (int i = 0; i < _n_features_to_add; i++)
				cout << features_to_add[i] << " ";
			cout << "\n";
			//int candidate_feature = findMaxFeatureGain(features_iter, threshold, features_converged);
			//cout << "iteration #" << features_iter << " added feature number "  << candidate_feature << ". ";
		}

		// Initialize for IIS or GIS
		bool not_converged = true;
		double held_out_likelihood = _LOG_OF_ZERO;
		double training_likelihood = _LOG_OF_ZERO;
		int iter = 0;
		for (int i = 0; i < _observedEvents->getNContexts(); i++) {
			for (int j = 0; j < _n_outcomes; j++) {
				_lastPredictedProbs[i][j] = 0;
			}
		}

		/*
		 *             IIS or GIS iteration section. 
		 * For each iteration, recalculate alphas, event predicted probs, then feature expectations.
		 */
		while (iter < 1000) { // (not_converged) {
			bool probs_not_converged = false;
			bool held_out_not_converged = false;

			findAlphas(iter);
			findPredictedProbs(probs_not_converged, threshold);
			findExpectations();
			findHeldOutLogLikelihood(held_out_not_converged, threshold, held_out_likelihood);

			if (_stopCriterion == PROBS_CONVERGE)
				not_converged = probs_not_converged;
			else if (_stopCriterion == HELD_OUT_LIKELIHOOD)
				not_converged = held_out_not_converged;

			if (fmodf((float)iter, 100.0) == 0) {
				_debug << "\nIteration " << iter << ":\n";
				cout << "Iteration " << iter << "\n";
				if (_debug.isActive()) {
					OldMaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
					while (fIter != _observedEvents->featureIterEnd()) {
						Symbol outcome = (*fIter).first->getOutcome();
						SymbolSet predicate = (*fIter).first->getPredicate();
						int featureID = _observedEvents->getFeatureID(outcome, predicate);
						if (_activeFeature[featureID])
							_debug << "  Alpha [" << featureID << " " << predicate.to_string() << " " << outcome.to_string() << "]: " << exp(_logAlphas[featureID]) << "\n";
						++fIter;
					}
					if (_trainMode == GIS)
						_debug << "  Alpha [CORRECTION_FEATURE]: " << exp(_logAlphas[_observedEvents->getNFeatures()]) << "\n\n";
					/*for (int i = 0; i < _observedEvents->getNContexts(); i++) {
						EventContext *context = _contexts[i];
						for (int j = 0; j < _n_outcomes; j++)
							_debug << "  P[" << i << "][" << j << "]: " << _predictedProbs[i][j] << "\n";
					}*/
					fIter = _observedEvents->featureIterBegin();
					while (fIter != _observedEvents->featureIterEnd()) {
						Symbol outcome = (*fIter).first->getOutcome();
						SymbolSet predicate = (*fIter).first->getPredicate();
						int featureID = _observedEvents->getFeatureID(outcome, predicate);
						_debug << "  O [" << featureID << " " << predicate.to_string() << " " << outcome.to_string() << "]: " << _observedFeatureProbs[featureID] << "\n";
						_debug << "  E [" << featureID << " " << predicate.to_string() << " " << outcome.to_string() << "]: " << _predictedExpectation[featureID] << "\n";
						++fIter;
					}
					_debug << "  O [CORRECTION_FEATURE]: " << _observedFeatureProbs[_observedEvents->getNFeatures()] << "\n";
					_debug << "  E [CORRECTION_FEATURE]: " << _predictedExpectation[_observedEvents->getNFeatures()] << "\n\n";
							
				}
			}

			iter++;
		}
		cout << iter << " iterations.\n";

		// Test for convergence of feature selection
		held_out_likelihood = findHeldOutLogLikelihood();
		training_likelihood = findTrainingLogLikelihood();
		cout << "held out data likelihood = " << held_out_likelihood << "\n";
		cout << "training data likelihood = " << training_likelihood << "\n";
		_debug << "held out data likelihood = " << held_out_likelihood << "\n";
		_debug << "training data likelihood = " << training_likelihood << "\n";

		if (_trainMode == IIS_FEATURE_SELECTION) {
			features_converged = false;		
			double diff = held_out_likelihood - last_held_out_likelihood;
			last_held_out_likelihood = held_out_likelihood;
			for (int t = 9; t > 0; t--) 
				last_ten_diffs[t] = last_ten_diffs[t-1];
			last_ten_diffs[0] = diff;
			double total = 0;
			bool all_negative = true;
			for (int s = 0; s < 10; s++) { 
				total += last_ten_diffs[s];
				if (last_ten_diffs[s] > 0)
					all_negative = false;
			}
			total = total / (double)5;
			if (all_negative && features_iter >= 10 && features_iter > (_n_outcomes * 2)) {
				features_converged = true;
				for (int t = 0; t < 10;  t++) 
					cout << last_ten_diffs[t] << " ";
				cout << "\n";
			}
		}

		features_iter++;
	}
	cout << "Done training\n";
	time_t end_time;
	time(&end_time);
	double total_time = difftime(end_time, start_time);
	cout << "Elapsed time " << total_time << " seconds.\n";
	cout << "Building alpha table...\n";

	buildAlphaTable();

	// clean up data structures
	for (int k = 0; k < _observedEvents->getNContexts(); k++) {
		for (int l = 0; l < _contexts[k]->getNOutcomes(); l++)
			delete [] _featureIDs[k][l];
		delete [] _predictedProbs[k];
		delete [] _lastPredictedProbs[k];
		delete [] _featureIDs[k];
		delete [] _predicateCounts[k];
		delete [] _observedEventProbs[k];
	}
	for (int m = 0; m < _heldOutEvents->getNContexts(); m++) {
		for (int n = 0; n < _heldOutContexts[m]->getNOutcomes(); n++) 
			delete [] _heldOutFeatureIDs[m][n];
		delete [] _heldOutPredictedProbs[m];
		delete [] _heldOutFeatureIDs[m];
		delete [] _heldOutPredicateCounts[m];
		delete [] _heldOutObservedEventProbs[m];
	}
	delete [] _observedFeatureProbs;
	delete [] _observedContextProbs;
	delete [] _logAlphas;
	delete [] _predictedExpectation;
	delete [] _predictedProbs;
	delete [] _lastPredictedProbs;
	delete [] _contexts;
	delete [] _featureIDs;
	delete [] _observedEventProbs;

	delete [] _predicateCounts;
	delete [] _featureOutcomes;
	delete [] _activeFeature;

	delete [] _heldOutPredictedProbs;
	delete [] _heldOutContexts;
	delete [] _heldOutFeatureIDs;
	delete [] _heldOutObservedEventProbs;
	delete [] _heldOutPredicateCounts;

}

void OldMaxEntModel::printModel(const char *prefix) { 
	char buffer[500];

	cout << "Writing Model file...\n";

	sprintf(buffer, "%s.maxent", prefix);
	UTF8OutputStream stream;
	stream.open(buffer);
	stream << _constantC << "\n";
	stream << _correctionAlpha << "\n";
	stream << _n_outcomes << "\n";
	for (int i = 0; i < _n_outcomes; i++)
		stream << _outcomes[i].to_string() << "\n";
	_alphaTable->print_to_open_stream(stream);
	stream.close();
}

void OldMaxEntModel::print_to_open_stream(UTF8OutputStream &stream) { 
	stream << _constantC << "\n";
	stream << _correctionAlpha << "\n";
	stream << _n_outcomes << "\n";
	for (int i = 0; i < _n_outcomes; i++)
		stream << _outcomes[i].to_string() << "\n";
	_alphaTable->print_to_open_stream(stream);
}

void OldMaxEntModel::calculateObservedProbs() {

	// correction feature observed prob will be determined as a result of this 
	int correctionFeatSum = 0;

	// calculate observedContextProbs and observedEventProbs
	for (int i = 0; i < _observedEvents->getNContexts(); i++) {
		EventContext *context = _contexts[i];
		_observedContextProbs[i] = context->getTotalCount() / (double)_observedEvents->getNEvents();
		for (int j = 0; j < _n_outcomes; j++) {
			_observedEventProbs[i][j] = 0;
			int k;	
			// find the index of _outcomes[j] in EventContext's list of outcomes
			for (k = 0; k < context->getNOutcomes(); k++) {
				if (context->getOutcome(k) == _outcomes[j]) 
					break;
			}
			// this outcome never occurs in training with context, so skip it 
			if (k == context->getNOutcomes()) 
				continue;
			_observedEventProbs[i][j] = context->getOutcomeCount(k) / (double)_observedEvents->getNEvents();
			if (_trainMode == GIS)
				correctionFeatSum += (_constantC - _predicateCounts[i][j]) * context->getOutcomeCount(k);
		}
  
	}
	
	// calculate observedFeatureProbs
	OldMaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
	for (int j = 0; j < _observedEvents->getNFeatures(); j++) {
		_observedFeatureProbs[(*fIter).first->getID()] = (*fIter).second/ (double)_observedEvents->getNEvents();
		++fIter;
	}
	if (_trainMode == GIS)
		_observedFeatureProbs[_observedEvents->getNFeatures()] = correctionFeatSum / (double)(_observedEvents->getNEvents() * _constantC);

	// calculate observedEventProbs for held out data
	for (int a = 0; a < _heldOutEvents->getNContexts(); a++) {
		EventContext *context = _heldOutContexts[a];
		for (int b = 0; b < _n_outcomes; b++) {
			// find the index of _outcomes[j] in EventContext's list of outcomes
			int c;	
			for (c = 0; c < context->getNOutcomes(); c++) {
				if (context->getOutcome(c) == _outcomes[b]) 
					break;
			}
			// this outcome never occurs in training with context, so skip it 
			if (c == context->getNOutcomes()) {
				_heldOutObservedEventProbs[a][b] = 0;
				continue;
			}
			_heldOutObservedEventProbs[a][b] = context->getOutcomeCount(c) / (double)_heldOutEvents->getNEvents();
		}
	}
}

void OldMaxEntModel::findAlphas(int iter) {
	if (_trainMode == GIS) 
		findAlphasGIS(iter);		
	else
		findAlphasIIS(iter);
}

void OldMaxEntModel::findAlphasGIS(int iter) {
	if (iter == 0)
		return;

	for (int i = 0; i < _observedEvents->getNFeatures() + 1; i++) {
		//_debug << "alpha[" << i << "][n] = " << exp(_logAlphas[i]) << " observed[" << i << "] = " << _observedFeatureProbs[i];
		//_debug << " predicted[" << i << "] = " << _predictedExpectation[i] << "\n";
		_logAlphas[i] = calculateAlphaGIS(_logAlphas[i], _observedFeatureProbs[i], _predictedExpectation[i]);
		//_debug << " alpha[" << i << "][n+1] = " << exp(_logAlphas[i]) << "\n";
	}
}

void OldMaxEntModel::initializeAlphas() {
	for (int i = 0; i < _observedEvents->getNFeatures() + 1; i++) {
		_logAlphas[i] = 0;
		_predictedExpectation[i] = 0;
	}
}


void OldMaxEntModel::findAlphasIIS(int iter) {
	if (iter == 0)
		return;

	for (int i = 0; i < _observedEvents->getNFeatures(); i++) {
		if (_activeFeature[i]) {
			//_debug << "alpha[" << i << "][n] = " << exp(_logAlphas[i]) << " observed[" << i << "] = " << _observedFeatureProbs[i];
			//_debug << " predicted[" << i << "] = " << _predictedExpectation[i] << "\n";
			_logAlphas[i] = calculateAlphaIIS(_logAlphas[i], _observedFeatureProbs[i], i);
			//_debug << "alpha[" << i << "][n+1] = " << exp(_logAlphas[i]) << "\n";
		}
	}
	//_debug << "\n\n";
}

double OldMaxEntModel::findHeldOutLogLikelihood(bool &not_converged, double threshold, double &last_likelihood) {
	double log_likelihood = findHeldOutLogLikelihood();

	double diff = (log_likelihood > last_likelihood) ? log_likelihood - last_likelihood : last_likelihood - log_likelihood;
	if (diff > threshold) 
		not_converged = true;
	last_likelihood = log_likelihood;
	return log_likelihood;
}


double OldMaxEntModel::findHeldOutLogLikelihood() {
	double log_likelihood = 0;

	// for each event seen in training, calculate a probability and store in a 2-D array, indexed firstly on the context for the event, and 
	// secondly on the outcome for the event.
	for (int i = 0; i < _heldOutEvents->getNContexts(); i++) {
		EventContext *context = _heldOutContexts[i];
		double contextSum = 0;

		for (int j = 0; j < _n_outcomes; j++) {

			double prob = 0;
			// for each feature, add in alpha
			// then add in correction features to make up space at end (for GIS only)
			for (int f = 0; f < context->getNPredicates(); f++) {
				if (_heldOutFeatureIDs[i][j][f] != -1 && _activeFeature[_heldOutFeatureIDs[i][j][f]]) {
					//_debug << "held out probs: feature #" << _heldOutFeatureIDs[i][j][f] << " added to context " << i;
					//_debug << " outcome " << j << "\n"; 
					prob += _logAlphas[_heldOutFeatureIDs[i][j][f]];
				}
			}
			_heldOutPredictedProbs[i][j] = prob;
			contextSum += exp(prob);
		}

		// Normalize each predicted prob by the total context sum, check for convergence
		for (int k = 0; k < _n_outcomes; k++) {
			_heldOutPredictedProbs[i][k] = exp(_heldOutPredictedProbs[i][k]) / contextSum;
			if (_heldOutObservedEventProbs[i][k] > 0) {
				log_likelihood += _heldOutObservedEventProbs[i][k] * LOG(_heldOutPredictedProbs[i][k]);
				//_debug << "held out log_likelihood += " << _heldOutObservedEventProbs[i][k] << " * LOG(" << _heldOutPredictedProbs[i][k] << ")";
				//_debug << " = " << _heldOutObservedEventProbs[i][k]  * LOG(_heldOutPredictedProbs[i][k]) << "\n";
			}
		}
	}

	return log_likelihood;
}

double OldMaxEntModel::findTrainingLogLikelihood() {

	double log_likelihood = 0;

	// for each event seen in training (_observedEventProbs[i][j] > 0), add 
	// to likelihood: observed prob of event * the log of event prob predicted by model   
	for (int i = 0; i < _observedEvents->getNContexts(); i++) {
		for (int j = 0; j < _n_outcomes; j++) {
			log_likelihood += _observedEventProbs[i][j] * LOG(_predictedProbs[i][j]);
		}
	}
	return log_likelihood;
}

void OldMaxEntModel::findPredictedProbs(bool &not_converged, double threshold) {

	double log_likelihood = 0;

	// for each context seen in training, calculate a probability for each possible outcome 
	// and store in a 2-D array, indexed firstly on the context for the event, and 
	// secondly on the outcome.
	for (int i = 0; i < _observedEvents->getNContexts(); i++) {
		EventContext *context = _contexts[i];
		double contextSum = 0;

		for (int j = 0; j < _n_outcomes; j++) {

			// for each active feature, add in alpha
			// then (for GIS only) add in correction features to make up for 
			// the difference between constant C and the # of active features
			_predictedProbs[i][j] = 0;
			for (int f = 0; f < context->getNPredicates(); f++) {
				if (_featureIDs[i][j][f] != -1 && _activeFeature[_featureIDs[i][j][f]]) 
					_predictedProbs[i][j] += _logAlphas[_featureIDs[i][j][f]];
			}
			if (_trainMode == GIS)
				_predictedProbs[i][j] += (_constantC - _predicateCounts[i][j]) * _logAlphas[_observedEvents->getNFeatures()];
			
			contextSum += exp(_predictedProbs[i][j]);
		}

		// Normalize each predicted prob by the total context sum, check for convergence
		for (int k = 0; k < _n_outcomes; k++) {
			_predictedProbs[i][k] = exp(_predictedProbs[i][k]) / contextSum;
			log_likelihood += _observedEventProbs[i][k] * LOG(_predictedProbs[i][k]);
			double diff = (_predictedProbs[i][k] > _lastPredictedProbs[i][k]) ? _predictedProbs[i][k] - _lastPredictedProbs[i][k] : _lastPredictedProbs[i][k] - _predictedProbs[i][k];
			if (diff > threshold) {
				not_converged = true;
				_lastPredictedProbs[i][k] = _predictedProbs[i][k];
			}
		}
	}
	_debug << "Training data log likelihood: " << log_likelihood << "\n";
}


void OldMaxEntModel::findExpectations() {

	for (int a = 0; a < _observedEvents->getNFeatures() + 1; a++) {
		_predictedExpectation[a] = 0;
	}

	// for each feature, calculate an expectation. It's easier to iterate over contexts
	// and just add to appropriate features
	for (int i = 0; i < _observedEvents->getNContexts(); i++) {
		EventContext *context = _contexts[i];
		for (int j = 0; j < _n_outcomes; j++) {
			for (int k = 0; k < context->getNPredicates(); k++) {
				// if there is a feature called (context[k], j), add the prob. of i,j * prob of i to 
				// that feature's expectation. 
				if (_featureIDs[i][j][k] != -1) //&& _activeFeature[_featureIDs[i][j][k]]) 
					_predictedExpectation[_featureIDs[i][j][k]] += (_observedContextProbs[i] * _predictedProbs[i][j]);
			}
			if (_trainMode == GIS) 
				_predictedExpectation[_observedEvents->getNFeatures()] += (_constantC - _predicateCounts[i][j]) * (_inverseC * _observedContextProbs[i] * _predictedProbs[i][j]);
		}
		
	}

}

double OldMaxEntModel::calculateAlphaGIS(double oldAlpha, double obsE, double predE) {
	double delta = _inverseC * (LOG(obsE) - LOG(predE));
	return oldAlpha + delta;
}

double OldMaxEntModel::calculateAlphaIIS(double oldAlpha, double obsE, int id) {
	double delta;

	if (_trainMode == IIS_GAUSSIAN)
		delta = newtonsMethodGaussian(obsE, id);
	else
		delta = newtonsMethod(obsE, id);

	return oldAlpha + delta;
}

// Use Newton's Method to solve for gamma:
// _observedFeatureProbs[i] - 
// Sum x,y ( _observedContextProbs[x] * _predictedProbs[x][y] * fi(x,y) * gamma^_predicateCounts[x][y] ) = 0
// fi(x,y) : returns 1 if feature i is active for given context x and outcome y
double OldMaxEntModel::newtonsMethod(double obsE, int id) {
	bool converged = false;
	double gamma = 1.1;
	double last_gamma;
	int j = _featureOutcomes[id];
	
	while (! converged ) {
		double numerator_sum = 0;
		double denominator_sum = 0;
		for (int i = 0; i < _observedEvents->getNContexts(); i++) {
			EventContext *context = _contexts[i]; 
			for (int k = 0; k < context->getNPredicates(); k++) {
				if (_featureIDs[i][j][k] == id) {				
					numerator_sum += _observedContextProbs[i] * _predictedProbs[i][j] * pow(gamma, _predicateCounts[i][j]);
					denominator_sum += _observedContextProbs[i] * _predictedProbs[i][j] * _predicateCounts[i][j] * pow(gamma, _predicateCounts[i][j] - 1);
					break;
				}
			}
		}
		last_gamma = gamma;
		gamma = gamma - ((obsE - numerator_sum) / -denominator_sum);
		double diff = (gamma > last_gamma) ? gamma - last_gamma : last_gamma - gamma;
		if (diff < 0.0001) {
			converged = true;
		}
		//_debug << obsE << " - " << numerator_sum << " = " << obsE - numerator_sum << " gamma: " << gamma << "\n";
	}
	//_debug << "gamma: " << gamma << "\n";
	return LOG(gamma);
}

// Use Newton's Method to solve for gamma:
// _observedFeatureProbs[i] - 
// Sum x,y ( _observedContextProbs[x] * _predictedProbs[x][y] * fi(x,y) * gamma^_predicateCounts[x][y] ) -
// (_logAlphas[i] + ln(gamma)) / variance = 0 
// fi(x,y) : returns 1 if feature i is active for given context x and outcome y
double OldMaxEntModel::newtonsMethodGaussian(double obsE, int id) {
	bool converged = false;
	double gamma = 1.1;
	double last_gamma;
	int j = _featureOutcomes[id];

	while (! converged ) {
		double numerator_sum = 0;
		double denominator_sum = 0;
		for (int i = 0; i < _observedEvents->getNContexts(); i++) {
			EventContext *context = _contexts[i];
			for (int k = 0; k < context->getNPredicates(); k++) {
				if (_featureIDs[i][j][k] == id) {				
					numerator_sum += _observedContextProbs[i] * _predictedProbs[i][j] * pow(gamma, _predicateCounts[i][j]);
					denominator_sum += _observedContextProbs[i] * _predictedProbs[i][j] * _predicateCounts[i][j] * pow(gamma, _predicateCounts[i][j] - 1);
				}
			}
		}
		//_debug << "numerator_sum: " << numerator_sum << "\n";
		//_debug << "logAlphas[i]: " << _logAlphas[id] << " log(gamma): " << LOG(gamma) << " variance: " << _variance << "\n";
		if (_variance != 0) {
			numerator_sum += (_logAlphas[id] + LOG(gamma)) / _variance;
			//_debug << "numerator_sum: " << numerator_sum << "\n";
			denominator_sum += (1 / (_variance * gamma));
		}
		last_gamma = gamma;
		gamma = gamma - ((obsE - numerator_sum) / -denominator_sum);
		double diff = (gamma > last_gamma) ? gamma - last_gamma : last_gamma - gamma;
		if (diff < 0.0001) {
			converged = true;
		}
		//_debug << obsE << " - " << numerator_sum << " = " << obsE - numerator_sum << " denominator_sum = " << denominator_sum << " gamma: " << gamma << "\n";
	}
	//_debug << "gamma: " << gamma << "\n";
	return LOG(gamma);
}


void OldMaxEntModel::buildAlphaTable() {
	OldMaxEntEventSet::FeatureIter iter = _observedEvents->featureIterBegin();
	while (iter != _observedEvents->featureIterEnd()) {
		Symbol outcome = (*iter).first->getOutcome();
		SymbolSet predicate = (*iter).first->getPredicate();
		int id = _observedEvents->getFeatureID(outcome, predicate);
		if (_activeFeature[id])
			_alphaTable->add(outcome, predicate, _logAlphas[id]);
		++iter;
	}
	_correctionAlpha = _logAlphas[_observedEvents->getNFeatures()];
}

int OldMaxEntModel::findMaxFeatureGain(int iter, double threshold, bool &converged) {

	if (iter == 0) {
		converged = false;
		return -1;
	}

	//_debug << "Entered findMaxFeatureGain\n";

	double *alpha = _new double[_observedEvents->getNFeatures()];
	double *gain = _new double[_observedEvents->getNFeatures()];
	gain[0] = -1000;
	int max_gain = 0;
	
	for (int i = 0; i < _observedEvents->getNFeatures(); i++) {
		if (_activeFeature[i] == 0) {
			// calculate alphas and gain
			alpha[i] = _observedFeatureProbs[i] * (1 - _predictedExpectation[i]);
			alpha[i] = alpha[i] / (_predictedExpectation[i] * (1 - _observedFeatureProbs[i]));
			alpha[i] = LOG(alpha[i]);
			gain[i] = calculateGain(i, alpha[i]);
			//_debug << "alpha[" << i << "] = " << alpha[i] << "\n";
			//_debug << "gain[" << i << "] = " << gain[i] << "\n";

			if (gain[i] > gain[max_gain])
				max_gain = i;
		}
	}

	if (gain[max_gain] > threshold) {
		converged = false;
		_activeFeature[max_gain] = 1;
		_logAlphas[max_gain]  = alpha[max_gain];
		_debug << "added feature " << max_gain << "\n";
		_debug << "approximate gain: " << gain[max_gain] << " from feature " << max_gain << "\n";
		
		// update predicate counts to include new feature
		ContextIter cIter = _observedEvents->contextIter();
		for (int i = 0; i < _observedEvents->getNContexts(); i++) {
			EventContext *context = cIter.findNext();
			for (int j = 0; j < _n_outcomes; j++) {
				for (int k = 0; k < context->getNPredicates(); k++) {
					if (_featureIDs[i][j][k] == max_gain)
							_predicateCounts[i][j]++;
				}	
			}
		}
		ContextIter hIter = _heldOutEvents->contextIter();
		for (int a = 0; a < _heldOutEvents->getNContexts(); a++) {
			EventContext *context = hIter.findNext();
			for (int b = 0; b < _n_outcomes; b++) {
				for (int c = 0; c < context->getNPredicates(); c++) {
					if (_heldOutFeatureIDs[a][b][c] == max_gain)
						_heldOutPredicateCounts[a][b]++;
				}	
			}
		}
	}
	
	delete [] alpha;
	delete [] gain;

	return max_gain;
}

int OldMaxEntModel::findMaxFeatureGain(int iter, double threshold, bool &converged, int *features, int max_features) {

	if (iter == 0) {
		converged = false;
		return -1;
	}

	//_debug << "Entered findMaxFeatureGain\n";

	double *alpha = _new double[_observedEvents->getNFeatures()];
	double *gain = _new double[_observedEvents->getNFeatures()];
	gain[0] = -1000;
	for (int a = 0; a < max_features; a++)
		features[a] = 0;
	
	for (int i = 0; i < _observedEvents->getNFeatures(); i++) {
		if (_activeFeature[i] == 0) {
			// calculate alphas and gain
			alpha[i] = _observedFeatureProbs[i] * (1 - _predictedExpectation[i]);
			alpha[i] = alpha[i] / (_predictedExpectation[i] * (1 - _observedFeatureProbs[i]));
			alpha[i] = LOG(alpha[i]);
			gain[i] = calculateGain(i, alpha[i]);
			//_debug << "alpha[" << i << "] = " << alpha[i] << "\n";
			//_debug << "gain[" << i << "] = " << gain[i] << "\n";

			if (gain[i] > gain[features[max_features - 1]]) {
				int iter = max_features - 2;
				while (iter >= 0 && gain[i] > gain[features[iter]])
					iter--;
				for (int j = max_features - 1; j > iter + 1; j--)
					features[j] = features[j - 1];
				features[iter + 1] = i;
			}
		}
	}

	//if (gain[features[0]] > threshold) {
		converged = false;
		for (int j = 0; j < max_features; j++) {
			_activeFeature[features[j]] = 1;
			_logAlphas[features[j]]  = alpha[features[j]];
			_debug << "added feature " << features[j] << "\n";
			_debug << "approximate gain: " << gain[features[j]] << " from feature " << features[j] << "\n";
		}
		// update predicate counts to include new feature
		ContextIter cIter = _observedEvents->contextIter();
		for (int i1 = 0; i1 < _observedEvents->getNContexts(); i1++) {
			EventContext *context = cIter.findNext();
			for (int j = 0; j < _n_outcomes; j++) {
				for (int k = 0; k < context->getNPredicates(); k++) {
					for (int l = 0; l < max_features; l++) {
						if (_featureIDs[i1][j][k] == features[l])
								_predicateCounts[i1][j]++;
					}
				}	
			}
		}
		ContextIter hIter = _heldOutEvents->contextIter();
		for (int a1 = 0; a1 < _heldOutEvents->getNContexts(); a1++) {
			EventContext *context = hIter.findNext();
			for (int b = 0; b < _n_outcomes; b++) {
				for (int c = 0; c < context->getNPredicates(); c++) {
					for (int d = 0; d < max_features; d++) {
						if (_heldOutFeatureIDs[a1][b][c] == features[d])
							_heldOutPredicateCounts[a1][b]++;
					}
				}	
			}
		}
	//}
	
	delete [] alpha;
	delete [] gain;

	return max_features;
}

double OldMaxEntModel::calculateGain(int feature_n, double alpha) {
	double gain = 0;
	
	for (int j = 0; j < _observedEvents->getNContexts(); j++) {
		EventContext *context = _contexts[j];
		double Z = 0;
		for (int k = 0; k < _n_outcomes; k++) {
			bool found = false;
			for (int l = 0; l < context->getNPredicates(); l++) {
				if (_featureIDs[j][k][l] == feature_n) {
					Z += _predictedProbs[j][k] * exp(alpha);
					found = true;
					break;
				}
			}
			if (!found) {
				Z += _predictedProbs[j][k];
			}
		}
		gain -= _observedContextProbs[j] * LOG(Z);
	}
	gain += alpha * _observedFeatureProbs[feature_n];
	
	return gain;
}

