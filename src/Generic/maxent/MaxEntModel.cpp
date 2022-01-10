// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/ParamReader.h"
#include "Generic/maxent/MaxEntModel.h"
#include "Generic/maxent/MaxEntEventSet.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include <math.h>
#include <time.h>
#include <set>
#include "boost/algorithm/string/replace.hpp"

#ifndef _LOG_OF_ZERO
#define _LOG_OF_ZERO -10000
#endif
#ifndef LOG
#define LOG(x) (x==0.0 ? _LOG_OF_ZERO : log(x))
#endif

 

MaxEntModel::MaxEntModel(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
							   DTFeature::FeatureWeightMap *weights, int train_mode,
							   int percent_held_out, int max_iterations, 
							   double variance, double min_likelihood_delta, 
							   int stop_check_freq,
							   const char *train_vector_file,
							   const char *held_out_vector_file) :
					_featureTypes(featureTypes), _tagSet(tagSet), 
					_weights(weights), _observedEvents(0), 
					_heldOutEvents(0), _trainMode(train_mode),
					_percentHeldOut(percent_held_out), _max_iterations(max_iterations),
					_min_likelihood_delta(min_likelihood_delta), _variance(variance),
					_stop_check_freq(stop_check_freq)
{ 
	std::string buffer = ParamReader::getParam("maxent_debug");
	DEBUG = (!buffer.empty());
	if (DEBUG) _debugStream.open(buffer.c_str());

	_observedEvents = _new MaxEntEventSet(tagSet, featureTypes, true, train_vector_file);
	_heldOutEvents = _new MaxEntEventSet(tagSet, featureTypes, false, held_out_vector_file);

	_n_events_added = 0;
}

MaxEntModel::~MaxEntModel() {
	_observedEvents->deallocateFeatures();
	delete _observedEvents;
	_heldOutEvents->deallocateFeatures();
	delete _heldOutEvents;
}

Symbol MaxEntModel::decodeToSymbol(DTObservation *observation) {
	int tag = decodeToInt(observation);
	return _tagSet->getTagSymbol(tag);
}

Symbol MaxEntModel::decodeToSymbol(DTObservation *observation, double& score, bool normalize_scores) {
	int tag = decodeToInt(observation, score, normalize_scores);
	return _tagSet->getTagSymbol(tag);
}

int MaxEntModel::decodeToInt(DTObservation *observation, double& finalscore, bool normalize_scores) {
	int n_tags = _tagSet->getNTags();
	
	double best_score = -10000000;
	int best_tag = -1;
	double second_best_score = -10000000;
	int second_best_tag = -1;

	double total_score = 0;

	for (int i = 0; i < n_tags; i++) {
		if (!observation->isValidTag(_tagSet->getTagSymbol(i)))
			continue;
		DTState state(_tagSet->getTagSymbol(i), Symbol(), 
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		double score = scoreState(state);
		if (normalize_scores && score != _LOG_OF_ZERO)
			total_score += exp(score);
		if (score > best_score) {
			second_best_score = best_score;
			second_best_tag = best_tag;
			best_score = score;
			best_tag = i;
		} else if (score > second_best_score) {
			second_best_score = score;
			second_best_tag = i;
		}  
	}

	// for now -- default
	if (best_tag == -1)
		best_tag = _tagSet->getNoneTagIndex();

	if (DEBUG) {
		_debugStream << "*********************************\n";
		printDebugInfo(observation, best_tag);
		_debugStream << L"\n";
		if (second_best_tag != best_tag) {
			printDebugInfo(observation, second_best_tag);
			_debugStream << L"\n";
		}
		if (best_tag != _tagSet->getNoneTagIndex() &&
			second_best_tag != _tagSet->getNoneTagIndex()) 
		{
			printDebugInfo(observation, _tagSet->getNoneTagIndex());
			_debugStream << L"\n";			
		}
		
	}

	finalscore = best_score;

	// NOTE: this is working in exp land now....
	if (normalize_scores) {
		if (total_score == 0 || finalscore == _LOG_OF_ZERO)
			finalscore = 0;
		else finalscore = exp(finalscore) / total_score;
	}

	return best_tag;
}

int MaxEntModel::decodeToInt(DTObservation *observation) {
	double finalscore;
	return decodeToInt(observation, finalscore);
}

int MaxEntModel::decodeToDistribution(DTObservation *observation, double *scores, 
									  int max_scores, int* best_tag) {
	double Z = 0;
	int n_tags = _tagSet->getNTags();

	if (max_scores < n_tags)
		throw InternalInconsistencyException("MaxEntModel::decodeToDistribution()",
											"Max scores is less than number of outcomes");
	
	for (int i = 0; i < n_tags; i++) {
		DTState state(_tagSet->getTagSymbol(i), Symbol(), 
			Symbol(), 0, std::vector<DTObservation*>(1, observation));
		scores[i] = exp(scoreState(state));
		Z += scores[i];

	}

	// normalize the scores so they all sum to one (note which one is the best)
	for (int j = 0; j < n_tags; j++)
		scores[j] = scores[j] / Z;

	if (best_tag) {
		double best_score = 0;
		*best_tag = 0;
		for (int k = 0; k < n_tags; k++) {
			if (scores[k] > best_score) {
				best_score = scores[k];
				*best_tag = k;
			}
		}
	}

	return n_tags;
}

double MaxEntModel::scoreState(const DTState &state) {
	double result = 0;

	for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, _featureBuffer);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = _featureBuffer[j];
			DTFeature::FeatureWeightMap::iterator iter = 
				_weights->find(feature);
			if (iter != _weights->end()) {
				// feature is in table, so use it and delete our copy of it
				result += *(*iter).second;
			}
			feature->deallocate();
		}
	}

	return result;
}

double MaxEntModel::scoreStateDuringDerivation(ObservationInfo *obsInfo, int tag) {
	double result = 0;

	for (int i = 0; i < obsInfo->getNFeatures(); i++) {
		DTFeature *feature = obsInfo->getFeature(i);
		int t = _tagSet->getTagIndex(feature->getTag());
		if (t != tag)
			continue;
		int id = _observedEvents->getFeatureID(feature);
		if (id != -1) {
			result += _logAlphas[id];
		}
	}

	return result;
}
void MaxEntModel::addToTraining(DTState state, int count) {
	if (_percentHeldOut > 0) {
		int n = 100 / _percentHeldOut; 
		if (_n_events_added % n == 0) 
			_heldOutEvents->addEvent(state, count);
		else 
			_observedEvents->addEvent(state, count);
	}
	else {
		_observedEvents->addEvent(state, count);
	}
	_n_events_added++;
}

void MaxEntModel::addToTraining(DTObservation *observation, int correct_answer) { 
	if (_percentHeldOut > 0) {
		int n = 100 / _percentHeldOut; 
		if (_n_events_added % n == 0) 
			_heldOutEvents->addEvent(observation, correct_answer);
		else 
			_observedEvents->addEvent(observation, correct_answer); 
	}
	else {
		_observedEvents->addEvent(observation, correct_answer); 
	}

	_n_events_added++;
}


void MaxEntModel::deriveModel(int pruning_threshold, bool const continuous_training) {
	if (_trainMode == GIS)
		deriveModelGIS(pruning_threshold, continuous_training);
	else if (_trainMode == SCGIS)
		deriveModelSCGIS(pruning_threshold, continuous_training);
}

void MaxEntModel::deriveModelGIS(int pruning_threshold, bool const continuous_training) { 
	
	// Prune
	_observedEvents->prune(pruning_threshold, _debugStream);

	// Get Constants
	_constantC = _observedEvents->getMaxActiveFeatures();
	_inverseC = (_constantC == 0) ? 0 : 1/(double)_constantC;
	_n_observations = _observedEvents->getNObservations();
	_n_features = _observedEvents->getNFeatures();

	if (DEBUG) {
		_debugStream << _observedEvents->getNEvents() << " total training instances.\n";
		_debugStream << _heldOutEvents->getNEvents() << " held out instances.\n";
		_debugStream << _n_features << " total features after pruning.\n\n";

		//_observedEvents->dump(_debugStream);
	}

	// Initialize data structures for training
	_s = _new double [_tagSet->getNTags()];
	_matrix = _new MatrixBlock*[_n_observations];
	_obsCount = _new int[_n_observations];
	_tagCount = _new int[_n_observations];
	_observed = _new double[_n_features];
	_expected = _new double[_n_features];
	_logAlphas = _new double[_n_features];
	

	// Store data in sparse matrix for faster lookup:
	// Each observation o has a linked list of linked lists containing 
	// features active in o, indexed by their tags (outcomes).
	ObsIter oIter = _observedEvents->observationIter();
	for (int i = 0; i < _n_observations; i++) {
		ObservationInfo *info = oIter.findNext();
		_obsCount[i] = info->getTotalCount();
		_tagCount[i] = 0;
		_matrix[i] = 0;

		for (int j = 0; j < info->getNFeatures(); j++) {
			DTFeature *feature = info->getFeature(j);
			int id = _observedEvents->getFeatureID(feature);
			int tag = _tagSet->getTagIndex(feature->getTag());
			// if feature exists in table, create a new feature block for id
			if (id != -1) {
				//search for this tag
				MatrixBlock *t_block = _matrix[i];
				while (t_block != 0 && t_block->id != tag)
					t_block = t_block->next;
				// if tag block doesn't exist, create a new tag block
				if (t_block == 0) {
					t_block = _new MatrixBlock(tag);
					t_block->next = _matrix[i];
					_matrix[i] = t_block;
					_tagCount[i]++;
				}
				// insert feature block id into tag's children
				MatrixBlock *f_block = _new MatrixBlock(id);
				f_block->next = t_block->child;
				t_block->child = f_block;
			}
		}
	}
	
	// initialize observed feature probs, log alphas and expectation
	MaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
	for (int f = 0; f < _n_features; f++) {
		_observed[(*fIter).first->getID()] = (*fIter).second;
		_logAlphas[(*fIter).first->getID()] = (continuous_training? *(*_weights)[(*fIter).first->getFeature()]: 0);
		_expected[(*fIter).first->getID()] = 0;
		++fIter;
	}

	// Initialize for GIS
	bool converged = false;
	double last_likelihood = -100000;
	int iter = 0;

	// Get start time
	time_t start_time;
	time(&start_time);
	
	while (!converged && iter < _max_iterations) {

		// for debugging purposes
		//for (int a = 0; a < _n_features; a++) {
		//	_expected[a] = 0;
		//}

	
		for (int j = 0; j < _n_observations; j++) {
			// initialize Z with 0 weight for each tag without any features
			double Z = _tagSet->getNTags() - _tagCount[j]; 
			// calculate outcome probs
			MatrixBlock *tag = _matrix[j];
			while (tag != 0) {
				_s[tag->id] = 0;
				MatrixBlock *feat = tag->child;
				while (feat != 0) {
					_s[tag->id] += _logAlphas[feat->id];
					feat = feat->next;
				}
				Z += exp(_s[tag->id]);
				tag = tag->next;
			}
			// calculate expected feature probs
			tag = _matrix[j];
			while (tag != 0) {
				MatrixBlock *feat = tag->child;
				double predicted = exp(_s[tag->id]) / Z;
				while (feat != 0) {
					_expected[feat->id] += (double)_obsCount[j] * predicted;
					feat = feat->next; 
				}
				tag = tag->next;
			}
			
		}


		// update alphas 
		for (int b = 0; b < _n_features; b++) {
			double delta = _inverseC * (LOG(_observed[b]) - LOG(_expected[b]));
			_logAlphas[b] += delta;
			_expected[b] = 0;
		}

		// Check stopping conditions
		if (iter % _stop_check_freq == 0) {
			if (_heldOutEvents->getNEvents() > 0) {
				double log_likelihood = findLogLikelihood(_heldOutEvents);
				double diff = (log_likelihood > last_likelihood) ? log_likelihood - last_likelihood : last_likelihood - log_likelihood;
				if (diff < (_min_likelihood_delta * _heldOutEvents->getNEvents())) 
					converged = true;
				last_likelihood = log_likelihood;
			}
			else {
				double log_likelihood = findLogLikelihood(_observedEvents);
				double diff = (log_likelihood > last_likelihood) ? log_likelihood - last_likelihood : last_likelihood - log_likelihood;
				if (diff < (_min_likelihood_delta * _observedEvents->getNEvents())) 
					converged = true;
				last_likelihood = log_likelihood;
			}
		}

		// output debug information
		/*if (!not_converged || (iter % 100 == 0)) {
			std::cout << "Iteration " << iter << "\n";
			if (DEBUG) {
				_debugStream << "\nIteration " << iter << ":\n";
				MaxEntObservationSet::FeatureIter fIter = _observedEvents->featureIterBegin();
				while (fIter != _observedEvents->featureIterEnd()) {
					DTFeature *feature = (*fIter).first->getFeature();
					int featureID = _observedEvents->getFeatureID(feature);
					_debugStream << "  Alpha [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _logAlphas[featureID] << "\n";
					++fIter;
				}
				fIter = _observedEvents->featureIterBegin();
				while (fIter != _observedEvents->featureIterEnd()) {
					DTFeature *feature = (*fIter).first->getFeature();
					int featureID = _observedEvents->getFeatureID(feature);
					_debugStream << "  O [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _observed[featureID] << "\n";
					_debugStream << "  E [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _expected[featureID] << "\n";
					double delta = _inverseC * (LOG(_observed[featureID]) - LOG(_expected[featureID]));
					_debugStream << "  delta [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << delta << "\n";
					++fIter;
				}		
			}
		}*/

		if (DEBUG) {
			_debugStream << "*** After iteration " << iter << " ***\n";
			double likelihood = findLogLikelihood(_observedEvents);
			_debugStream << "Log likelihood of training data = " << likelihood << "\n";
			likelihood = findLogLikelihood(_heldOutEvents);
			_debugStream << "Log likelihood of held out data = " << likelihood << "\n";
			double percent = findPercentCorrect(_observedEvents);
			_debugStream << "Percent correct on training data = " << percent * 100 << "\n";
			percent = findPercentCorrect(_heldOutEvents);
			_debugStream << "Percent correct on held out data = " << percent * 100 << "\n";
			double f_measure = findFMeasure(_observedEvents);
			_debugStream << "F measure on training data = " << f_measure << "\n";
			f_measure = findFMeasure(_heldOutEvents);
			_debugStream << "F measure on held out data = " << f_measure << "\n";
		}
		iter++;
	}
	SessionLogger::info("SERIF") << iter << " iterations.\n";		
	SessionLogger::info("SERIF") << "Done training\n";

	// Report time
	time_t end_time;
	time(&end_time);
	double total_time = difftime(end_time, start_time);
	SessionLogger::info("SERIF") << "Elapsed time " << total_time << " seconds.\n";

	// Build Weights Table
	SessionLogger::info("SERIF") << "Building weights table...\n";
	MaxEntEventSet::FeatureIter it = _observedEvents->featureIterBegin();
	while (it != _observedEvents->featureIterEnd()) {
		int id = _observedEvents->getFeatureID((*it).first->getFeature());
		// gives _weights ownership of feature and sets value
		*(*_weights)[(*it).first->getFeature()] = _logAlphas[id];
		++it;
	}

	if (DEBUG) {
		_debugStream << "*** After iteration " << iter << " ***\n";
		double likelihood = findLogLikelihood(_observedEvents);
		_debugStream << "Log likelihood of training data = " << likelihood << "\n";
		likelihood = findLogLikelihood(_heldOutEvents);
		_debugStream << "Log likelihood of held out data = " << likelihood << "\n";
		double percent = findPercentCorrect(_observedEvents);
		_debugStream << "Percent correct on training data = " << percent * 100 << "\n";
		percent = findPercentCorrect(_heldOutEvents);
		_debugStream << "Percent correct on held out data = " << percent * 100 << "\n";
		double f_measure = findFMeasure(_observedEvents);
		_debugStream << "F measure on training data = " << f_measure << "\n";
		f_measure = findFMeasure(_heldOutEvents);
		_debugStream << "F measure on held out data = " << f_measure << "\n";
	}

	// clean up data structures
	for (int k = 0; k < _n_observations; k++) {
		MatrixBlock *tag_ptr = _matrix[k];
		MatrixBlock *last_tag;
		while (tag_ptr != 0) {
			MatrixBlock *feat_ptr = tag_ptr->child;
			MatrixBlock *last_feat;
			while (feat_ptr != 0) {
				last_feat = feat_ptr;
				feat_ptr = feat_ptr->next;
				delete last_feat;
			}
			last_tag = tag_ptr;
			tag_ptr = tag_ptr->next;
			delete last_tag;
		}
	}
	
	delete [] _expected;
	delete [] _logAlphas;
	delete [] _observed;
	delete [] _tagCount;
	delete [] _obsCount;
	delete [] _matrix;
	delete [] _s;
}

void MaxEntModel::deriveModelSCGIS(int pruning_threshold, bool const continuous_training) { 
	
	// Prune
	_observedEvents->prune(pruning_threshold, _debugStream);

	// Get Constants
	_n_observations = _observedEvents->getNObservations();
	_n_features = _observedEvents->getNFeatures();

	if (DEBUG) {
		_debugStream << _observedEvents->getNEvents() << " total training instances.\n";
		_debugStream << _heldOutEvents->getNEvents() << " held out instances.\n";
		_debugStream << _n_features << " total features after pruning.\n\n";

		//_observedEvents->dump(_debugStream);
	}

	// Initialize data structures for training
	_S = _new double *[_n_observations];
	_Z = _new double[_n_observations];
	_obsCount = _new int[_n_observations];
	_matrix = _new MatrixBlock *[_n_features];
	_observed = _new double[_n_features];
	_expected = _new double[_n_features];
	_logAlphas = _new double[_n_features];

	// initialize observed feature probs, log alphas, and feature matrix
	MaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
	for (int f = 0; f < _n_features; f++) {
		_observed[(*fIter).first->getID()] = (*fIter).second;
		_logAlphas[f] = (continuous_training? *(*_weights)[(*fIter).first->getFeature()]: 0);
		_matrix[f] = 0;
		++fIter;
	}

	// Store data in sparse matrix for faster lookup:
	// Each feature f has a linked list of linked lists containing 
	// observations where f is active, indexed by their tags (outcomes).  
	ObsIter oIter = _observedEvents->observationIter();
	for (int i = 0; i < _n_observations; i++) {
		_S[i] = _new double[ _tagSet->getNTags()];
		ObservationInfo *info = oIter.findNext();
		_obsCount[i] = info->getTotalCount();
		for (int j = 0; j < info->getNFeatures(); j++) {
			DTFeature *feature = info->getFeature(j);
			int id = _observedEvents->getFeatureID(feature);
			int tag = _tagSet->getTagIndex(feature->getTag());
			// if feature exists in table, create a new observation block for i
			if (id != -1) {
				// look for the tag block with id tag
				MatrixBlock *t_block = _matrix[id];
				while (t_block != 0 && t_block->id != tag)
					t_block = t_block->next;
				// if tag block doesn't exist, create a new tag block
				if (t_block == 0) {
					t_block = _new MatrixBlock(tag);
					t_block->next = _matrix[id];
					_matrix[id] = t_block;
				}
				// add new observation block i to tag block's children
				MatrixBlock *o_block = _new MatrixBlock(i);
				o_block->next = t_block->child;
				t_block->child = o_block;
			}
		}
	}

	// Initialize for SCGIS
	bool converged = false;
	double last_likelihood = -100000;
	int iter = 0;
	
	if (!continuous_training) {
		for (int c = 0; c < _n_observations; c++) {
			_Z[c] = _tagSet->getNTags();
			for (int d = 0; d < _tagSet->getNTags(); d++) {
				_S[c][d] = 0;
			}
		}
	} else {
		for (int c = 0; c < _n_observations; c++) {
			_Z[c] = _tagSet->getNTags();
			for (int d = 0; d < _tagSet->getNTags(); d++) {
				_S[c][d] = 0;
			}
		}
		for (int f = 0; f < _n_features; f++) {
			MatrixBlock *tag = _matrix[f];
			while (tag != 0) {
				MatrixBlock *obs = tag->child;
				while (obs != 0) {
					_Z[obs->id] -= exp(_S[obs->id][tag->id]);
					_S[obs->id][tag->id] += _logAlphas[f];
					_Z[obs->id] += exp(_S[obs->id][tag->id]);
					obs = obs->next;
				}
				tag = tag->next;
			}
		}
	}

	// Get start time
	time_t start_time;
	time(&start_time);

	while (!converged && iter < _max_iterations) {
		std::cout << ".";
		std::cout.flush();
		for (int i = 0; i < _n_features; i++) {
			//double expected = 0;
			_expected[i] = 0;
			MatrixBlock *tag = _matrix[i];
			while (tag != 0) {
				MatrixBlock *obs = tag->child;
				while (obs != 0) {
					_expected[i] += _obsCount[obs->id] * ((exp(_S[obs->id][tag->id]) / _Z[obs->id]));
					obs = obs->next;
				}
				tag = tag->next;
			}
			double delta;
			if (_variance != 0)
				delta = newtonsMethodGaussianSCGIS(i);
			else {
				delta = LOG(_observed[i]) - LOG(_expected[i]);
			}
			_logAlphas[i] += delta;
			tag = _matrix[i];
			while (tag != 0) {
				MatrixBlock *obs = tag->child;
				while  (obs != 0) {
					_Z[obs->id] -= exp(_S[obs->id][tag->id]);
					_S[obs->id][tag->id] += delta;
					_Z[obs->id] += exp(_S[obs->id][tag->id]);
					obs = obs->next;
				}
				tag = tag->next;
			}
		}

		// Check stopping conditions
		if (iter % _stop_check_freq == 0) {
			if (_heldOutEvents->getNEvents() > 0) {
				double log_likelihood = findLogLikelihood(_heldOutEvents);
				double diff = (log_likelihood > last_likelihood) ? log_likelihood - last_likelihood : last_likelihood - log_likelihood;
				if (diff < (_min_likelihood_delta * _heldOutEvents->getNEvents())) 
					converged = true;
				last_likelihood = log_likelihood;
			}
			else {
				double log_likelihood = findLogLikelihood(_observedEvents);
				double diff = (log_likelihood > last_likelihood) ? log_likelihood - last_likelihood : last_likelihood - log_likelihood;
				if (diff < (_min_likelihood_delta * _observedEvents->getNEvents())) 
					converged = true;
				last_likelihood = log_likelihood;
			}
		}

		// output debug information
		/*if (converged || (iter % 100 == 0)) {
			std::cout << "Iteration " << iter << "\n";
			if (DEBUG) {
				_debugStream << "\nIteration " << iter << ":\n";
				MaxEntEventSet::FeatureIter fIter = _observedEvents->featureIterBegin();
				while (fIter != _observedEvents->featureIterEnd()) {
					DTFeature *feature = (*fIter).first->getFeature();
					int featureID = _observedEvents->getFeatureID(feature);
					_debugStream << "  Alpha [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _logAlphas[featureID] << "\n";
					++fIter;
				}
				fIter = _observedEvents->featureIterBegin();
				while (fIter != _observedEvents->featureIterEnd()) {
					DTFeature *feature = (*fIter).first->getFeature();
					int featureID = _observedEvents->getFeatureID(feature);
					_debugStream << "  O [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _observed[featureID] << "\n";
					_debugStream << "  E [" << featureID << " ";
					_debugStream << feature->getFeatureType()->getName().to_string() << " ";
					feature->write(_debugStream);
					_debugStream << "]: " << _expected[featureID] << "\n";
					++fIter;
				}		
			}
		}*/

		if (DEBUG && (iter % 10 == 0)) {
			_debugStream << "*** After iteration " << iter << " ***\n";
			double likelihood = findLogLikelihood(_observedEvents);
			_debugStream << "Log likelihood of training data = " << likelihood << "\n";
			likelihood = findLogLikelihood(_heldOutEvents);
			_debugStream << "Log likelihood of held out data = " << likelihood << "\n";
			double percent = findPercentCorrect(_observedEvents);
			_debugStream << "Percent correct on training data = " << percent * 100 << "\n";
			percent = findPercentCorrect(_heldOutEvents);
			_debugStream << "Percent correct on held out data = " << percent * 100 << "\n";
			double f_measure = findFMeasure(_observedEvents);
			_debugStream << "F measure on training data = " << f_measure << "\n";
			f_measure = findFMeasure(_heldOutEvents);
			_debugStream << "F measure on held out data = " << f_measure << "\n";
		}

		iter++;
	}
	SessionLogger::info("SERIF") << iter << " iterations.\n";		
	SessionLogger::info("SERIF") << "Done training\n";

	// Report time
	time_t end_time;
	time(&end_time);
	double total_time = difftime(end_time, start_time);
	SessionLogger::info("SERIF") << "Elapsed time " << total_time << " seconds.\n";

	// Build Weights Table
	SessionLogger::info("SERIF") << "Building weights table...\n";
	MaxEntEventSet::FeatureIter it = _observedEvents->featureIterBegin();
	while (it != _observedEvents->featureIterEnd()) {
		int id = _observedEvents->getFeatureID((*it).first->getFeature());
		// gives _weights ownership of feature and sets value
		*(*_weights)[(*it).first->getFeature()] = _logAlphas[id];
		++it;
	}

	if (DEBUG) {
		_debugStream << "*** After iteration " << iter << " ***\n";
		double likelihood = findLogLikelihood(_observedEvents);
		_debugStream << "Log likelihood of training data = " << likelihood << "\n";
		likelihood = findLogLikelihood(_heldOutEvents);
		_debugStream << "Log likelihood of held out data = " << likelihood << "\n";
		double percent = findPercentCorrect(_observedEvents);
		_debugStream << "Percent correct on training data = " << percent * 100 << "\n";
		percent = findPercentCorrect(_heldOutEvents);
		_debugStream << "Percent correct on held out data = " << percent * 100 << "\n";
		double f_measure = findFMeasure(_observedEvents);
		_debugStream << "F measure on training data = " << f_measure << "\n";
		f_measure = findFMeasure(_heldOutEvents);
		_debugStream << "F measure on held out data = " << f_measure << "\n";
	}

	// clean up data structures
	for (int k = 0; k < _n_features; k++) {
		MatrixBlock *tag_ptr = _matrix[k];
		MatrixBlock *last_tag;
		while (tag_ptr != 0) {
			MatrixBlock *obs_ptr = tag_ptr->child;
			MatrixBlock *last_obs;
			while (obs_ptr != 0) {
				last_obs = obs_ptr;
				obs_ptr = obs_ptr->next;
				delete last_obs;
			}
			last_tag = tag_ptr;
			tag_ptr = tag_ptr->next;
			delete last_tag;
		}
	}
	for (int c1 = 0; c1 < _n_observations; c1++) {
		delete _S[c1];
	}
	delete [] _logAlphas;
	delete [] _expected;
	delete [] _observed;
	delete [] _matrix;
	delete [] _obsCount;
	delete [] _Z;
	delete [] _S;
}

// Use Newton's Method to solve for gamma:
// _observed[i] - 
// (_expected[i] * gamma^max fi(x,y)) -
// (_logAlphas[i] + ln(gamma)) / variance = 0 
// fi(x,y) : returns 1 if feature i is active for given context x and outcome y
double MaxEntModel::newtonsMethodGaussianSCGIS(int id) {
	bool converged = false;
	double gamma = 1.1;
	double last_gamma;

	while (! converged ) {
		double numerator_sum = _expected[id] * gamma;
		double denominator_sum = _expected[id];
		if (_variance != 0) {
			numerator_sum += (_logAlphas[id] + LOG(gamma)) / _variance;
			//_debug << "numerator_sum: " << numerator_sum << "\n";
			denominator_sum += (1 / (_variance * gamma));
		}
		last_gamma = gamma;
		gamma = gamma - ((_observed[id] - numerator_sum) / -denominator_sum);
		double diff = (gamma > last_gamma) ? gamma - last_gamma : last_gamma - gamma;
		if (diff < 0.0001) {
			converged = true;
		}
		//_debug << obsE << " - " << numerator_sum << " = " << obsE - numerator_sum << " denominator_sum = " << denominator_sum << " gamma: " << gamma << "\n";
	}
	//_debug << "gamma: " << gamma << "\n";
	return LOG(gamma);
}


/*double MaxEntModel::findLogLikelihood(MaxEntEventSet *observations) {

	double log_likelihood = 0;

	ObsIter oIter = observations->observationIter();
	for (int i = 0; i < observations->getNObservations(); i++) {
		ObservationInfo *info = oIter.findNext();
		double Z = 0;
		for (int j = 0; j < _tagSet->getNTags(); j++) {
			double score = scoreStateDuringDerivation(info, j);
			Z += exp(score);
		}
		for (int t = 0; t < info->getNOutcomes(); t++) {
			int correct_tag = info->getOutcome(t);
			double score = scoreStateDuringDerivation(info, correct_tag, true);
			// observed prob of x,y
			double obs_prob_xy = (info->getOutcomeCount(t) / (double)observations->getNEvents());
			// observed prob times log of (prob of y|x)
			log_likelihood += obs_prob_xy * LOG(exp(score)/Z); 
		}
	}
	return log_likelihood;
}*/

double MaxEntModel::findLogLikelihood(MaxEntEventSet *observations) {

	double log_likelihood = 0;
	double *scores = _new double[_tagSet->getNTags()];

	ObsIter oIter = observations->observationIter();
	for (int i = 0; i < observations->getNObservations(); i++) {
		ObservationInfo *info = oIter.findNext();
		double max = -1000;
		for (int j = 0; j < _tagSet->getNTags(); j++) {
			scores[j] = scoreStateDuringDerivation(info, j);
			if (scores[j] > max)
				max = scores[j];
		}
		double Z = 0;
		// scale score for more accurate calculation of log (added to agree with Mallet)
		for (int f = 0; f < _tagSet->getNTags(); f++) 
			Z += (scores[f] = exp(scores[f] - max));
		for (int t = 0; t < info->getNOutcomes(); t++) {
			int correct_tag = info->getOutcome(t);
			// observed prob of x,y
			//double obs_prob_xy = (info->getOutcomeCount(t) / (double)observations->getNEvents());
			// observed prob times log of (prob of y|x)
			// Note: now using count instead of probability to agree with Mallet
			log_likelihood -= info->getOutcomeCount(t) * LOG(scores[correct_tag]/Z); 
		}
	}

	
	delete [] scores;
	return -log_likelihood;
}

double MaxEntModel::findPercentCorrect(MaxEntEventSet *observations) {
	int correct = 0;

	if (observations->getNEvents() == 0)
		return 0;

	ObsIter oIter = observations->observationIter();
	for (int i = 0; i < observations->getNObservations(); i++) {
		ObservationInfo *info = oIter.findNext();
		double best_score = -1000;
		int best_tag = -1;
		for (int j = 0; j < _tagSet->getNTags(); j++) {
			double score = scoreStateDuringDerivation(info, j);
			if (score > best_score) {
				best_score = score;
				best_tag = j;
			}
		}
		for (int t = 0; t < info->getNOutcomes(); t++) {
			int correct_tag = info->getOutcome(t);
			if (best_tag == correct_tag) 
				correct += info->getOutcomeCount(t);
		}
	}
	return (correct / (double) observations->getNEvents());
}

double MaxEntModel::findFMeasure(MaxEntEventSet *observations) {
	int correct = 0;
	int spurious = 0;
	int missed = 0;
	int wrong_type = 0;

	if (observations->getNEvents() == 0)
		return 0;

	ObsIter oIter = observations->observationIter();
	for (int i = 0; i < observations->getNObservations(); i++) {
		ObservationInfo *info = oIter.findNext();
		double best_score = -1000;
		int best_tag = -1;
		for (int j = 0; j < _tagSet->getNTags(); j++) {
			double score = scoreStateDuringDerivation(info, j);
			if (score > best_score) {
				best_score = score;
				best_tag = j;
			}
		}
		for (int t = 0; t < info->getNOutcomes(); t++) {
			int correct_tag = info->getOutcome(t);
			if (best_tag != _tagSet->getNoneTagIndex() ||
				correct_tag != _tagSet->getNoneTagIndex()) 
			{
				if (best_tag == correct_tag)
					correct += info->getOutcomeCount(t);
				else if (correct_tag == _tagSet->getNoneTagIndex())
					spurious += info->getOutcomeCount(t);
				else if (best_tag == _tagSet->getNoneTagIndex())
					missed += info->getOutcomeCount(t);
				else if (best_tag != correct_tag)
					wrong_type += info->getOutcomeCount(t);
			}
		}
	}
	double recall = (double) correct / (missed + wrong_type + correct);
	double precision = (double) correct / (spurious + wrong_type + correct);
	return ((2 * recall * precision) / (recall + precision));

}

std::wstring MaxEntModel::getDebugInfo(DTObservation *observation, int answer) {
	std::wstringstream result;
	double score = 0;
	DTState state(_tagSet->getTagSymbol(answer), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));

	result << _tagSet->getTagSymbol(answer).to_string() << L":\n";

	for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, _featureBuffer);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = _featureBuffer[j];
			std::wstring wstr;
			feature->toString(wstr);
			result << feature->getFeatureType()->getName().to_string() << L" ";
			result << wstr;
			result << L": ";
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(feature);
			if (iter != _weights->end()) {
				result << *(*iter).second;
				score += *(*iter).second;
			} else result << "NOT IN TABLE";
			result << L"\n";
			feature->deallocate();
		}
	}
	result << L"SCORE: " << score << L"\n";
	return result.str();
}

void MaxEntModel::printDebugInfo(DTObservation *observation, int answer) {
	if (answer < 0)
		return;
	if (DEBUG) {
		_debugStream << getDebugInfo(observation, answer);
	}
}

void MaxEntModel::printDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug){
	debug << getDebugInfo(observation, answer);
}

void MaxEntModel::printDebugInfo(DTObservation *observation, int answer, DebugStream& debug){
	debug << getDebugInfo(observation, answer);
}

void MaxEntModel::printHTMLDebugInfo(DTObservation *observation, int answer, 
									 UTF8OutputStream& debug)
{
	double score = 0;
	DTState state(_tagSet->getTagSymbol(answer), Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));
	for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, _featureBuffer);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = _featureBuffer[j];
			debug << feature->getFeatureType()->getName().to_string() << L" ";
			std::wstring str;
			feature->toString(str);
			for (unsigned ch = 0; ch < str.length(); ch++) {
				if (str.at(ch) == L'<')
					str.replace(ch, 1, L"[");
				if (str.at(ch) == L'>')
					str.replace(ch, 1, L"]");
			}
			debug << str;
			debug << L": ";
			DTFeature::FeatureWeightMap::iterator iter = _weights->find(feature);
			if (iter != _weights->end()) {
				debug << *(*iter).second;
				score += *(*iter).second;
			} else debug << "NOT IN TABLE";
			debug << L"<br>\n";
			feature->deallocate();
		}
	}
}

// This nameless namespace is used to declare this class to be file-local.  It
// is just intended to be used by the printDebugTable method, defined below.
namespace {
	/** A class used to sort features by their relative "importance" in deciding the 
	  * tag for a given observation.  The importance is currently calculated as the
	  * maximum difference between feature weights over pairs of tags. */
	struct debugTableSorter {
		typedef Symbol::HashMap<std::map<std::wstring, double> > FeatureTableMap;
		std::set<Symbol> &tags;
		FeatureTableMap &featureTableMap;

		debugTableSorter(FeatureTableMap &m, std::set<Symbol> &tags): featureTableMap(m), tags(tags) {}

		double importance(std::wstring feature) const {
			double result = 0;
			for (std::set<Symbol>::iterator t1=tags.begin(); t1 != tags.end(); ++t1)
				for (std::set<Symbol>::iterator t2=tags.begin(); t2 != tags.end(); ++t2)
					result = std::max(result, std::abs(featureTableMap[*t1][feature] - featureTableMap[*t2][feature]));
			return result;
		}

		bool operator() (std::wstring f1, std::wstring f2) const { 
			return importance(f1) > importance(f2);
		}
	};
}

void MaxEntModel::printDebugTable(DTObservation *observation, UTF8OutputStream& debug){
	int n_tags = _tagSet->getNTags();
	Symbol::HashMap<std::map<std::wstring, double> > featureTableMap; // tag -> feature -> weight.
	Symbol::HashMap<double> score; // Total score for each tag
	std::set<Symbol> tags; // set of all tags
	std::set<std::wstring> featureNames; // set of all feature names (sorted alphabetically)

	// Populate the table.
	for (int tag=0; tag<n_tags; ++tag) {
		Symbol tagSymbol = _tagSet->getTagSymbol(tag); 
		tags.insert(tagSymbol);
		DTState state(tagSymbol, Symbol(), Symbol(), 0, std::vector<DTObservation*>(1, observation));
		for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
			int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, _featureBuffer);
			// I'm assuming here that there's one feature per tag, and that they line up.. Is that ok?
			for (int j = 0; j < n_features; j++) {
				DTFeature *feature = _featureBuffer[j];

				// Get a name for this feature.
				std::wstring featureString;
				feature->toStringWithoutTag(featureString);
				std::wstring featureTypeName = feature->getFeatureType()->getName().to_string();
				featureString = featureTypeName+L"("+featureString+L")";
				featureNames.insert(featureString);

				// Record the weight for this feature.
				DTFeature::FeatureWeightMap::iterator iter = _weights->find(feature);
				if (iter != _weights->end()) {
					double weight = *(*iter).second;
					featureTableMap[tagSymbol][featureString] = weight;
					score[tagSymbol] += weight;
				} else {
					featureTableMap[tagSymbol][featureString] = 0;
				}
				feature->deallocate();
			}
		}
	}

	// Get a list of all feature names, sorted by their "importance".
	std::vector<std::wstring> sortedFeatureNames(featureNames.begin(), featureNames.end());
	std::sort(sortedFeatureNames.begin(), sortedFeatureNames.end(), debugTableSorter(featureTableMap, tags));
	
	// Header:
	debug << L"MAXENT FEATURE TABLE:\n\t";
	for (std::set<Symbol>::iterator tIter=tags.begin(); tIter != tags.end(); ++tIter)
		debug << L"\t" << tIter->to_string();
	debug << L"\n";

	// Table body:
	for (size_t i=0; i<sortedFeatureNames.size(); ++i) {
		std::wstring featureName = sortedFeatureNames[i];
		debug << L"\t" << featureName;
		for (std::set<Symbol>::iterator tIter=tags.begin(); tIter != tags.end(); ++tIter)
			debug << L"\t" << featureTableMap[*tIter][featureName];
		debug << L"\n";
	}

	// Footer:
	debug << "\tTOTAL:";
	for (std::set<Symbol>::iterator tIter=tags.begin(); tIter != tags.end(); ++tIter) {
		debug << L"\t" << score[*tIter];
	}
	debug << L"\n";
}
