// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discTagger/DTTagSet.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTFeatureTypeSet.h"
#include "Generic/discTagger/PWeight.h"
#include "Generic/discTagger/PDecoder.h"
#include "Generic/discTagger/BlockFeatureTable.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/SessionLogger.h"

#include <cfloat>
#include <sstream>

#include <boost/format.hpp>
#include <boost/foreach.hpp>

using namespace std;

PDecoder::PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   DTFeature::FeatureWeightMap *weights, bool add_hyp_features, bool use_lazy_sum)
	: _add_hyp_features(add_hyp_features), _tagSet(tagSet),
	  _featureTypes(featureTypes), _weights(weights), _weightsByBlock(0),
	  _observationOnlyFeatureTypes(0), _withPrevTagFeatureTypes(0), 
	  _n_observation_only_feature_types(0), _n_with_prev_tag_feature_types(0), 
	  _use_lazy_sum(use_lazy_sum), _n_examples(0),
	  _n_observation_only_buffered_features(0),_n_with_prev_tag_buffered_features(0)

{
	// Split feature types to speed up the decoding.
	splitFeatureTypes();

	_use_buffered_features = false;
	_max_buffered_features = 0;
	_n_buffered_tags = 0;
	_observationOnlyFeatureBuffer = 0;
	_withPrevTagFeatureBuffer = 0;
}

PDecoder::PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   BlockFeatureTable *weights, bool add_hyp_features, bool use_lazy_sum)
	: _add_hyp_features(add_hyp_features), _tagSet(tagSet),
	  _featureTypes(featureTypes), _weights(0), _weightsByBlock(weights),
	  _observationOnlyFeatureTypes(0), _withPrevTagFeatureTypes(0), 
	  _n_observation_only_feature_types(0), _n_with_prev_tag_feature_types(0), 
	  _use_lazy_sum(use_lazy_sum), _n_examples(0),
	  _n_observation_only_buffered_features(0),_n_with_prev_tag_buffered_features(0)

{
	// Split feature types to speed up the decoding.
	splitFeatureTypes();

	// Initialize feature buffers
	_use_buffered_features = true;
	_max_buffered_features = DTFeatureType::MAX_FEATURES_PER_EXTRACTION * _featureTypes->getNFeaturesTypes();
	_n_buffered_tags = 0;
	_observationOnlyFeatureBuffer = 0;
	_withPrevTagFeatureBuffer = 0;
	initFeatureBuffers();
}

PDecoder::~PDecoder(){
	delete[] _observationOnlyFeatureTypes;
	delete[] _withPrevTagFeatureTypes;
	freeFeatureBuffers();
}

void PDecoder::initFeatureBuffers() {
	// Are we using buffered features? (This code path is the default)
	if (_use_buffered_features) {
		// (Re-)initialize if the number of tags has changed
		if (_tagSet->getNTags() != _n_buffered_tags) {
			freeFeatureBuffers();
			_n_buffered_tags = _tagSet->getNTags();
			_observationOnlyFeatureBuffer = _new double*[_max_buffered_features];
			_withPrevTagFeatureBuffer = _new double*[_max_buffered_features];
			for (int i = 0; i < _max_buffered_features; i++) {
				_observationOnlyFeatureBuffer[i] = _new double[_n_buffered_tags];
				_withPrevTagFeatureBuffer[i] = _new double[_n_buffered_tags];
				memset(_observationOnlyFeatureBuffer[i], 0, _n_buffered_tags*sizeof(double));
				memset(_withPrevTagFeatureBuffer[i], 0, _n_buffered_tags*sizeof(double));
			}
		}
	} else {
		_withPrevTagFeatureBuffer = 0;
		_observationOnlyFeatureBuffer = 0;
	}
}

void PDecoder::freeFeatureBuffers() {
	if (_observationOnlyFeatureBuffer && _withPrevTagFeatureBuffer) {
		for (int i = 0; i < _max_buffered_features; i++) {
			delete[] _observationOnlyFeatureBuffer[i];
			delete[] _withPrevTagFeatureBuffer[i];
		}
		delete[] _observationOnlyFeatureBuffer;
		delete[] _withPrevTagFeatureBuffer;
	}
}

void PDecoder::decode(std::vector<DTObservation *> & observations, Symbol *tags) {
	int *tag_ints = _new int[observations.size()];

	decode(observations, tag_ints);

    convertTagsToSymbols(static_cast<int>(observations.size()), tag_ints, tags);

	delete[] tag_ints;
}

double PDecoder::decode(std::vector<DTObservation *> & observations, int *tags) {
	return constrainedDecode(observations, NULL, tags);
}


double PDecoder::constrainedDecode(std::vector<DTObservation*> & observations, 
                                 int* constraints, int *tags)
{
	int n_tags = _tagSet->getNTags();
	int arraysz = static_cast<int>(observations.size())*n_tags;
	double *f_score = _new double[arraysz];
	bool *f_active = _new bool[arraysz];
	int *traceback = _new int[arraysz];


	forwardPass(observations, traceback, f_score, f_active, constraints);
	tracePath(static_cast<int>(observations.size()), traceback, tags);

	if (DEBUG)
		printTrellis(observations, tags, f_score, f_active);

	double score = f_score[(observations.size()-1)*n_tags+tags[observations.size()-1]];

	delete[] f_active;
	delete[] f_score;
	delete[] traceback;

	return score;
}


void PDecoder::addFeatures(std::vector<DTObservation *> & observations,
						   int *answer, int n_obs_to_ignore)
{
	int n_obs = static_cast<int>(observations.size());
	if (_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::addFeatures()",
				"Cannot add new features to buffered feature table structure");
	}
	
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	//MRF: try to match Java DT, the java implementation
	// ignores observations based on START and END observations for namefinding
	for (int i = n_obs_to_ignore; i < (n_obs - n_obs_to_ignore); i++) {
		// construct state
		int tag = answer[i];
		int prev_tag = answer[i-1];
		const Symbol &rTag = _tagSet->getReducedTagSymbol(tag);
		const Symbol &srTag = _tagSet->getSemiReducedTagSymbol(tag);
		DTState state (_tagSet->getTagSymbol(tag), rTag,srTag,
					  _tagSet->getTagSymbol(prev_tag), i,
					  observations);
		
		// extract features from that state
		for (int k = 0; k < _featureTypes->getNFeaturesTypes(); k++) {
			int n_features = _featureTypes->getFeatureType(k)
									->extractFeatures(state, featureArray);
			for (int j = 0; j < n_features; j++) {
				DTFeature *feature = featureArray[j];
				PWeight *value = _weights->get(feature);
				if (value == 0) {
					*(*_weights)[feature] = 0;
				} else {
					// the feature is already in the hash table, so delete this
					// copy of it
					feature->deallocate();
				}
			}
		}
	}
}


bool PDecoder::train(std::vector<DTObservation *> &  observations, int *answer, int n_obs_to_ignore)
{
	int n_obs = static_cast<int>(observations.size());
    int *hypothesis = _new int[n_obs];
    bool *correct_inds = _new bool[n_obs]; // holds inds where the hypo is correct
	_n_examples++;

	double best_hypo_score = decode(observations, hypothesis);

	bool correct = true;

	//This is to ignore completely the errors in the n_obs_to_ignore
	int i, j;
	for (i = 0, j = n_obs - 1; i < n_obs_to_ignore; i++, j--) {
		answer[i] = hypothesis[i];
		answer[j] = hypothesis[j];
	}
	for (i = 0; i < n_obs; i++) {
		correct_inds[i] = true;
	}
	for (i = 0; i < n_obs; i++) {
		if (hypothesis[i] != answer[i]) {
			correct = false;
			correct_inds[i] = false;
			// if you are wrong, you also influence the decision of the next position.
			if (i < (n_obs - 1)) {
				correct_inds[++i] = false;
			}
		}
	}

	if (!correct) {
		double delta = 1; // the amount to update the weights
		adjustWeights(observations, answer, delta, true, n_obs_to_ignore, correct_inds);
		adjustWeights(observations, hypothesis, -delta, _add_hyp_features, n_obs_to_ignore, correct_inds);
	}
	delete[] hypothesis;
	delete[] correct_inds;

	return correct;
}


double PDecoder::trainWithMargin(std::vector<DTObservation *> & observations, 
					 int *answer, int n_obs_to_ignore,
					 double required_margin)
{
	int n_obs = static_cast<int>(observations.size());
	int *best_tags = _new int[n_obs];
	int *secondbest_tags = _new int[n_obs];
	bool *correct_inds = _new bool[n_obs]; // holds inds where the hypo is correct

	// initialize the tag arrays
	memset(best_tags, 0, sizeof(int) * n_obs);
	memset(secondbest_tags, 0, sizeof(int) * n_obs);

	double margin = 0;
	double best_score = 0;
	double secondbest_score = 0;
	_n_examples++;

	decodeAllTags(observations, 0, best_tags, secondbest_tags, best_score, secondbest_score);

	bool correct = true;
	for (int i = 0; i < n_obs; i++) {
		if (best_tags[i] != answer[i]) {
			correct = false;
			correct_inds[i] = false;
		} else {
			correct_inds[i] = true;
		}
	}

	int* wrong_tags = 0;
	if (!correct) {
		double correct_answer_score = scorePath(observations, answer);
		margin = correct_answer_score - best_score;
		wrong_tags = best_tags;
	} else { // the best_path is the correct path
		margin = best_score-secondbest_score;
		wrong_tags = secondbest_tags;
		for (int i = 0; i < n_obs; i++) {
			if (wrong_tags[i] != answer[i]) {
				correct_inds[i] = false;
			} else{
				correct_inds[i] = true;
			}
		}
	}

	if (margin < required_margin) {
		double delta = 1.0; // the amount to update the weights by
		adjustWeights(observations, answer, delta, true, n_obs_to_ignore, correct_inds);
		adjustWeights(observations, wrong_tags, -delta, _add_hyp_features, n_obs_to_ignore, correct_inds);
	}
	delete[] best_tags;
	delete[] secondbest_tags;
	delete[] correct_inds;

	return margin;
}

double PDecoder::scorePath(std::vector<DTObservation *> & observations, int *answer) {
	double tempscore = 0;
	double score = 0;
	int n_obs = static_cast<int>(observations.size());
	for (int prev_index = 0; prev_index < n_obs - 1; prev_index++) {
		int index = prev_index + 1;
		int prev_tag = answer[prev_index];
		int tag = answer[index];

		const Symbol &rTag = _tagSet->getReducedTagSymbol(tag);
		const Symbol &srTag = _tagSet->getSemiReducedTagSymbol(tag);
		DTState state(_tagSet->getTagSymbol(tag), rTag,srTag,
				_tagSet->getTagSymbol(prev_tag), index,
				observations);

		tempscore = scoreState(state);
		score += tempscore;
	}

	return score;
}

double PDecoder::decodeAllTags(std::vector<DTObservation *> & observations, int n_obs_to_ignore,
                               int* best_tags, int* secondbest_tags,
                               int& best_position, 
                               double& the_best_score, double& the_secondbest_score)
{
	int n_tags = _tagSet->getNTags();
	int arraysz = static_cast<int>(observations.size())*n_tags;
	double *f_score = _new double[arraysz];
	bool *f_active = _new bool[arraysz];
	double *b_score = _new double[arraysz];
	bool *b_active = _new bool[arraysz];
	int *traceback = _new int[arraysz];
	int *traceforward = _new int[arraysz];
	for (int i = 0; i< arraysz; i++) {
		f_active[i] = false;
		b_active[i] = false;
		f_score[i] = 0;
		b_score[i] = 0;
		traceback[i] = -1;
		traceforward[i] = -1;
	}

	forwardPass(observations, traceback, f_score, f_active);
	backwardPass(observations, traceforward, b_score, b_active);

	int best_tag = -1;
	int secondbest_tag = -1;
	double best_score = -DBL_MAX;
	double secondbest_score = -DBL_MAX;

	int minmargin_pos = 0;
	double minmargin = DBL_MAX; 
	int minmargin_tag = 0; 
	int minmargin_secondbest_tag = 0;
	Symbol marginbest = Symbol(L"not-set");
	Symbol marginsecondbest = Symbol(L"not-set");  

	// We want to find the best and secondbest tag (and scores) for 
	// each observation. We also want to identify the observation
	// with the least margin between best and secondbest scores.
	int n_obs = static_cast<int>(observations.size());
	for (int k = n_obs_to_ignore; k < n_obs-n_obs_to_ignore; k++) {
		best_score = -DBL_MAX;
		secondbest_score = -DBL_MAX;
		best_tag = -1;
		secondbest_tag = -1;

		// find the best and secondbest tags
		for (int j = 0; j < n_tags; j++) {
			if (f_active[k*n_tags+j] && b_active[k*n_tags+j]) {
				double this_score = f_score[k*n_tags+j] + b_score[k*n_tags+j];
				if (this_score > secondbest_score) {
					if (this_score > best_score) {
						secondbest_score = best_score;
						secondbest_tag = best_tag;
						best_score = this_score;
						best_tag = j;
					}
					else {
						secondbest_score = this_score;
						secondbest_tag = j;
					}
				}
			}
		}

		// calculate the margin between best and secondbest
		double this_margin = best_score - secondbest_score;
		if (this_margin < minmargin) {
			if (best_tag >= 0)
				marginbest = _tagSet->getTagSymbol(best_tag);
			if (secondbest_tag >= 0)
				marginsecondbest = _tagSet->getTagSymbol(secondbest_tag);
			minmargin_pos = k;
			minmargin = this_margin;
			minmargin_tag = best_tag;
			minmargin_secondbest_tag = secondbest_tag;
			the_best_score = best_score;
			the_secondbest_score = secondbest_score;
		}
	}

	// get the best path
	if (best_tags != 0) {
		tracePath(n_obs, traceback, best_tags);
	}

	// get the second best path
	if (secondbest_tags != 0) {
		tracePathFromMiddle(n_obs, traceback, traceforward, 
                                    minmargin_pos, minmargin_secondbest_tag, 
                                    secondbest_tags);
	}

	best_position = minmargin_pos;

	delete[] traceback;
	delete[] traceforward;
	delete[] b_active;
	delete[] b_score;
	delete[] f_active;
	delete[] f_score;

	return minmargin;

}


double PDecoder::decodeAllTags(std::vector<DTObservation *> & observations,  int n_obs_to_ignore, int* tag_ints) {
	int best_position;
	double best_score, secondbest_score;
	return decodeAllTags(observations, n_obs_to_ignore, 
                             tag_ints, 0, best_position, 
                             best_score, secondbest_score);
}


double PDecoder::decodeAllTagsPos(std::vector<DTObservation *> & observations, int& best_position) {
	double best_score, secondbest_score;
	return decodeAllTags(observations, 0, 0, 0, 
                             best_position, best_score, secondbest_score);
}

double PDecoder::decodeAllTags(std::vector<DTObservation *> & observations,  
                             int n_obs_to_ignore, int* best_tags, int* secondbest_tags,
                             double &best_score, double &secondbest_score) 
{
	int best_position;
	return decodeAllTags(observations, n_obs_to_ignore, best_tags, secondbest_tags, best_position, best_score, secondbest_score);
}


/** Note: I don't really understand what the second part of this method
 *  is supposed to be doing and how it works, so I'm going to leave
 *  it alone and not collapse it with other decodeAllTags methods.
 *
 *  JSM, 06/19/2009
 */  
double PDecoder::decodeAllTags(std::vector<DTObservation *> & observations,  
                               double* scores, int n_scores,
                               int n_obs_to_ignore, int* tag_ints)
{
	int n_tags = _tagSet->getNTags();
	int arraysz = static_cast<int>(observations.size())*n_tags;
	double *f_score = _new double[arraysz];
	bool *f_active = _new bool[arraysz];
	double *b_score = _new double[arraysz];
	bool *b_active = _new bool[arraysz];
	int *traceback = _new int[arraysz];
	int *traceforward = _new int[arraysz];
	for (int i = 0; i< arraysz; i++) {
		f_active[i] = false;
		b_active[i] = false;
		f_score[i] = 0;
		b_score[i] = 0;
		traceback[i] = -1;
		traceforward[i] = -1;
	}

	forwardPass(observations, traceback, f_score, f_active);
	backwardPass(observations, traceforward, b_score, b_active);

	int best_tag = -1;
	int secondbest_tag = -1;
	double best_score = -DBL_MAX;
	double secondbest_score = -DBL_MAX;

	int minmargin_pos = 0;
	double minmargin = DBL_MAX;
	int minmargin_tag = 0;
	Symbol marginbest = Symbol(L"not-set");
	Symbol marginsecondbest = Symbol(L"not-set");  

	// We want to find the best and secondbest tag (and scores) for 
	// each observation. We also want to identify the observation
	// with the least margin between best and secondbest scores.
	int n_obs = static_cast<int>(observations.size());
	for (int k = n_obs_to_ignore; k < n_obs-n_obs_to_ignore; k++) {
		best_score = -DBL_MAX;
		secondbest_score = -DBL_MAX;
		best_tag = -1;
		secondbest_tag = -1;

		// find the best and secondbest tags
		for (int j = 0; j < n_tags; j++) {
			if (f_active[k*n_tags+j] && b_active[k*n_tags+j]) {
				double this_score = f_score[k*n_tags+j] + b_score[k*n_tags+j];
				if (this_score > secondbest_score) {
					if (this_score > best_score) {
						secondbest_score = best_score;
						secondbest_tag = best_tag;
						best_score = this_score;
						best_tag = j;
					}
					else {
						secondbest_score = this_score;
						secondbest_tag = j;
					}
				}
			}
		}
		//std::cout<<"\tword: "<<k<<" best score: "<<best_score<<" forward: "
		//	<<f_score[k*n_tags+best_tag]<<" backward: "<<b_score[k*n_tags+best_tag]
		//	<<" secondbest score: "<<secondbest_score	<<" forward: "
		//	<<b_score[k*n_tags+secondbest_tag]<<" backward: "<<b_score[k*n_tags+secondbest_tag]
		//	<<std::endl;

		// calculate the margin between best and secondbest
		double this_margin = best_score - secondbest_score;
		if (this_margin < minmargin) {
			if (best_tag >= 0)
				marginbest = _tagSet->getTagSymbol(best_tag);
			if (secondbest_tag >= 0)
				marginsecondbest = _tagSet->getTagSymbol(secondbest_tag);
			minmargin_pos = k;
			minmargin = this_margin;
			minmargin_tag = best_tag;
		}
	}
	//std::cout<<"minmargin: "<<minmargin<<" between "<<marginbest.to_debug_string()
	//	<<" and "<<marginsecondbest.to_debug_string()<<" at position "
	//	<<minmargin_pos<< " of "<<n_obs<<" best_forward: "
	//	<<f_score[minmargin_pos*n_tags+minmargin_tag]
	//	<<" best back_ward: "<<b_score[minmargin_pos*n_tags+minmargin_tag]
	//	<<std::endl;

  
	//cheat to get a list of all tags, get the tags that can follow NONE-ST,
	//this should be all ST tags except NONE and NONE-CO
	//ff you are running in a mode where tags are learned from the training this may not be true
	int none_st = _tagSet->getNoneTagIndex();
	std::set<int> obsTags = _tagSet->getSuccessorTags(none_st);
	const Symbol &none_sym = _tagSet->getReducedTagSymbol(none_st);


	int n_counted = 0;
	BOOST_FOREACH(int this_tag, obsTags) {
		const Symbol &red_tag = _tagSet->getReducedTagSymbol(this_tag);
		if ((red_tag == none_sym) ||
			(this_tag == _tagSet->getEndTagIndex()) ||
			(this_tag == _tagSet->getStartTagIndex()) ) continue;
		//std::cout<<"Adding: "<< _tagSet->getTagSymbol(thistag).to_debug_string() <<" as "<<ncounted<<std::endl;

		if (n_counted >= n_scores) {
			ostringstream ostr;
			ostr << "more tags than space: "<<n_counted <<" "<<n_scores<<std::endl;
			BOOST_FOREACH(int obsTag, obsTags) {
				ostr<<"tag: "<< _tagSet->getTagSymbol(obsTag).to_debug_string() <<" as "<<n_counted<<std::endl;
			}
			SessionLogger::err("SERIF") << ostr.str();
			continue;
		}
		for (int k = n_obs_to_ignore; k < n_obs-n_obs_to_ignore; k++) {
			best_score = -DBL_MAX;
			secondbest_score = -DBL_MAX;
			best_tag = -1;
			secondbest_tag = -1;
			
			for (int j = 0; j < n_tags; j++) {
				if (f_active[k*n_tags+j] && b_active[k*n_tags+j]) {
					double this_score = f_score[k*n_tags+j] + b_score[k*n_tags+j];
					if (this_score > secondbest_score) {
						if (this_score > best_score) {
							secondbest_score = best_score;
							secondbest_tag = best_tag;
							best_score = this_score;
							best_tag = j;
						}
						else {
							secondbest_score = this_score;
							secondbest_tag = j;

						}
					}
				}
			}
			//std::cout<<"\tword: "<<k<<" best score: "<<best_score<<" forward: "
			//	<<f_score[k*n_tags+best_tag]<<" backward: "<<b_score[k*n_tags+best_tag]
			//	<<" secondbest score: "<<secondbest_score	<<" forward: "
			//	<<b_score[k*n_tags+secondbest_tag]<<" backward: "<<b_score[k*n_tags+secondbest_tag]
			//	<<std::endl;
			//calculate the margin
			double this_margin = best_score - secondbest_score;
			if ((this_margin < minmargin) && (_tagSet->getReducedTagSymbol(best_tag) == red_tag)){
				if (best_tag >= 0)
					marginbest = _tagSet->getTagSymbol(best_tag);
				if (secondbest_tag >= 0)
					marginsecondbest = _tagSet->getTagSymbol(secondbest_tag);
				minmargin_pos = k;
				minmargin = this_margin;
				minmargin_tag = best_tag;
			}
		}
		if (minmargin < 100000000) {
			scores[n_counted] = minmargin;
		}
		else {
			scores[n_counted] = -1;
		}
		n_counted++;

	}

	// get the best path
	if (tag_ints != 0) {
		tracePath(n_obs, traceback, tag_ints);
	}

	delete[] traceforward;
	delete[] traceback;
	delete[] b_active;
	delete[] b_score;
	delete[] f_active;
	delete[] f_score;

	return minmargin;
}

double PDecoder::constrainedDecodeAllTags(std::vector<DTObservation *> & observations,  int* constraints,
										  int* tag_ints)
{
	int n_obs_to_ignore = 0; //this used to be a parameter
	int n_tags = _tagSet->getNTags();

	int arraysz = static_cast<int>(observations.size())*n_tags;
	double *f_score = _new double[arraysz];
	bool *f_active = _new bool[arraysz];
	double *b_score = _new double[arraysz];
	bool *b_active = _new bool[arraysz];
	int *traceback = _new int[arraysz];
	int *traceforward = _new int[arraysz];
	for (int i = 0; i < arraysz; i++) {
		f_active[i] = false;
		b_active[i] = false;
		f_score[i] = 0;
		b_score[i] = 0;
		traceback[i] = -1;
		traceforward[i] = -1;
	}

	forwardPass(observations, traceback, f_score, f_active, constraints);
	backwardPass(observations, traceforward, b_score, b_active, constraints);

	int best_tag = -1;
	int secondbest_tag = -1;
	double best_score = -DBL_MAX;
	double secondbest_score = -DBL_MAX;


	int minmargin_pos = 0;
	double minmargin = DBL_MAX;
	int minmargin_tag = 0;
	Symbol marginbest = Symbol(L"not-set");
	Symbol marginsecondbest = Symbol(L"not-set");

	int n_obs = static_cast<int>(observations.size());
	for (int k = n_obs_to_ignore; k < n_obs-n_obs_to_ignore; k++) {
		best_score = -DBL_MAX;
		secondbest_score = -DBL_MAX;
		best_tag = -1;
		secondbest_tag =-1;

		// only look at margins for non constrained tags
		if (constraints[k] == -1) {
			for (int j = 0; j < n_tags; j++) {
				if(f_active[k*n_tags+j] && b_active[k*n_tags+j]){
					double this_score = f_score[k*n_tags+j] + b_score[k*n_tags+j];
					if (this_score > secondbest_score) {
						if (this_score > best_score) {
							secondbest_score = best_score;
							secondbest_tag = best_tag;
							best_score = this_score;
							best_tag = j;
						}
						else {
							secondbest_score = this_score;
							secondbest_tag = j;

						}
					}
				}
			}

			//calculate the margin
			double this_margin = best_score - secondbest_score;
			if (this_margin < minmargin) {
				if (best_tag >= 0)
					marginbest = _tagSet->getTagSymbol(best_tag);
				if (secondbest_tag >= 0)
					marginsecondbest = _tagSet->getTagSymbol(secondbest_tag);
				minmargin_pos = k;
				minmargin = this_margin;
				minmargin_tag = best_tag;
			}
		}
	}

	//get the best path
	if (tag_ints != 0) {
		tracePath(n_obs, traceback, tag_ints);
	}

	if (DEBUG) {
		printTrellis(observations, tag_ints, f_score, f_active);
		printTrellis(observations, tag_ints, b_score, b_active);
	}

	delete[] traceforward;
	delete[] traceback;
	delete[] b_active;
	delete[] b_score;
	delete[] f_active;
	delete[] f_score;
	return minmargin;
}

void PDecoder::forwardPass(std::vector<DTObservation *> & observations,
						   int *traceback)
{
	int n_obs = static_cast<int>(observations.size());
	int n_tags = _tagSet->getNTags();
	double *f_score = _new double[n_obs*n_tags];
	bool *f_active = _new bool[n_obs*n_tags];
	forwardPass(observations, traceback, f_score, f_active);
	delete[] f_active;
	delete[] f_score;
}

/*
	f_score and f_active must be of size n_obs*n_tags
*/
// forwardPass was fixed to take the advantage of the grouping of the feature types.
// Due to this, the way it loops over the previous tags and current tags is rearranged,
// and, hence, different from the other decoding rountine.
void PDecoder::forwardPass(vector<DTObservation *> & observations,
				 		   int *traceback, double* f_score,
						   bool* f_active, int* constraints)
{
	int n_obs = static_cast<int>(observations.size());
	int n_tags = _tagSet->getNTags();
	int n_regular_tags = _tagSet->getNRegularTags();
	initFeatureBuffers();

	for (int i = 0; i < n_obs; i++) {
		for (int j = 0; j < n_tags; j++) {
			f_active[n_tags*i + j] = false;
		}
	}

	int start_tag = _tagSet->getStartTagIndex();
	f_active[start_tag] = true;
	f_score[start_tag] = 0;

	Symbol fakePrevTag;

	// Process all but the last (END) observation - do that one separately
	for (int prev_index = 0; prev_index < n_obs - 2; prev_index++) {
		int index = prev_index + 1;

		bool is_first_active_tag = true;

		for (int tag = 0; tag < n_regular_tags; tag++) {

			if (constraints != NULL && !isAllowableTag(tag, constraints[index])) 
						continue;

			std::set<int> prev_tags = _tagSet->getPredecessorTags(tag);
			if (prev_tags.size() <= 0)
				continue;

			const Symbol &rTag = _tagSet->getReducedTagSymbol(tag);
			const Symbol &srTag = _tagSet->getSemiReducedTagSymbol(tag);			

			bool at_least_one_predecessor = false;

			BOOST_FOREACH(int prev_tag, prev_tags) {
				if (constraints != NULL && !isAllowableTag(prev_tag, constraints[prev_index])) 
						continue;

				if (f_active[n_tags*prev_index + prev_tag]) {

					DTState curState(_tagSet->getTagSymbol(tag), rTag, srTag,
									_tagSet->getTagSymbol(prev_tag), index,
									observations);
					
					if (_use_buffered_features) {
						loadWithPrevTagFeatureBuffer(curState, _withPrevTagFeatureTypes, _n_with_prev_tag_feature_types);
					}

					double score = f_score[n_tags*prev_index + prev_tag]
						+ scoreWithPrevTagState(curState, _withPrevTagFeatureTypes, _n_with_prev_tag_feature_types);
					
					if ( !f_active[n_tags*index + tag] ||
						score > f_score[n_tags*index + tag])
					{
						f_score[n_tags*index + tag] = score;
						traceback[n_tags*index + tag] = prev_tag;
						f_active[n_tags*index + tag] = true;
						at_least_one_predecessor = true;
					}

				}
			}
			if (at_least_one_predecessor) {
				
				DTState state(_tagSet->getTagSymbol(tag), rTag, srTag,
							fakePrevTag, index,
							observations);
				
				if (_use_buffered_features && is_first_active_tag) {
					loadObservationOnlyFeatureBuffer(state, _observationOnlyFeatureTypes, _n_observation_only_feature_types);
					is_first_active_tag = false;
				}

				f_score[n_tags*index + tag] += scoreObservationOnlyState(state, _observationOnlyFeatureTypes, _n_observation_only_feature_types);
				
			}
		}
	}

	// Process the last observation on its own.  The only valid tag is "END",
	// so we don't need to score, just update the traceback to the highest prev tag
	int prev_index = n_obs - 2;
	int index = prev_index + 1;
	int tag = _tagSet->getEndTagIndex();

	std::set<int> prev_tags = _tagSet->getPredecessorTags(tag);

	BOOST_FOREACH(int prev_tag, prev_tags) {
		if (f_active[n_tags*prev_index + prev_tag]) {
			double score = f_score[n_tags*prev_index + prev_tag];
			if ( !f_active[n_tags*index + tag] ||
				score > f_score[n_tags*index + tag])
			{
				f_score[n_tags*index + tag] = score;
				traceback[n_tags*index + tag] = prev_tag;
				f_active[n_tags*index + tag] = true;
			}
		}
	}

}

void PDecoder::backwardPass(std::vector<DTObservation *> & observations)
{
	int n_obs = static_cast<int>(observations.size());
	int n_tags = _tagSet->getNTags();
	double *b_score = _new double[n_obs*n_tags];
	bool *b_active = _new bool[n_obs*n_tags];
	int *traceforward = _new int[n_obs*n_tags];
	backwardPass(observations, traceforward, b_score, b_active);
	delete[] traceforward;
	delete[] b_active;
	delete[] b_score;
}

/*
	b_score and b_active must be of size n_obs*n_tags
*/
void PDecoder::backwardPass(std::vector<DTObservation *> & observations,
                            int* traceforward, double* b_score,
                            bool* b_active, int* constraints)
{
	int n_obs = static_cast<int>(observations.size());
	int n_tags = _tagSet->getNTags();
	initFeatureBuffers();

	for (int i = 0; i < n_obs; i++) {
		for (int j = 0; j < n_tags; j++) {
			b_active[n_tags*i + j] = false;
		}
	}

	int end_tag = _tagSet->getEndTagIndex();
	b_active[n_tags*(n_obs-1)+end_tag] = true;
	b_score[n_tags*(n_obs-1)+end_tag] = 0;

	Symbol fakePrevTag;

	for (int index = n_obs-1; index > 0; index--) {
		int prev_index = index - 1;
		bool is_first_active_tag = true;
		for (int tag = 0; tag < n_tags; tag++) {
	
			if (b_active[index*n_tags + tag]) {

				std::set<int> prev_tags = _tagSet->getPredecessorTags(tag);
				if (prev_tags.size() <= 0)
					continue;

				const Symbol &rTag = _tagSet->getReducedTagSymbol(tag);
				const Symbol &srTag = _tagSet->getSemiReducedTagSymbol(tag);
				DTState state(_tagSet->getTagSymbol(tag), rTag, srTag,
							  fakePrevTag, index,
							  observations); 

				if (_use_buffered_features && is_first_active_tag) {
					loadObservationOnlyFeatureBuffer(state, _observationOnlyFeatureTypes, _n_observation_only_feature_types);
					is_first_active_tag = false;
				}

				double observation_only_score = scoreObservationOnlyState(state, _observationOnlyFeatureTypes, _n_observation_only_feature_types);

				BOOST_FOREACH(int prev_tag, prev_tags) {
					if (constraints != NULL && !isAllowableTag(prev_tag, constraints[prev_index]))
						continue;

					DTState curState(_tagSet->getTagSymbol(tag), rTag, srTag,
					                 _tagSet->getTagSymbol(prev_tag), index,
					                 observations); 

					if (_use_buffered_features) {
						loadWithPrevTagFeatureBuffer(curState, _withPrevTagFeatureTypes, _n_with_prev_tag_feature_types);
					}



					double score = b_score[n_tags*index + tag] + observation_only_score
								   + scoreWithPrevTagState(curState, _withPrevTagFeatureTypes, _n_with_prev_tag_feature_types);


					if (!b_active[n_tags*prev_index + prev_tag] ||
					     score > b_score[n_tags*prev_index + prev_tag])
					{
						b_score[n_tags*prev_index + prev_tag] = score;
						b_active[n_tags*prev_index + prev_tag] = true;
						traceforward[n_tags*prev_index + prev_tag] = tag;
					}
					
				}
			}
		}
	}
}

double PDecoder::scoreState(const DTState &state) {
	double result = 0;

	if (_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::scoreState()",
				"This version of the scoreState method is incompatible with the buffered features setting.");
	}

	if (DEBUG)
		printDebugStateInfo(state);

	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	for (int i = 0; i < _featureTypes->getNFeaturesTypes(); i++) {
		int n_features = _featureTypes->getFeatureType(i)->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			PWeight *value = _weights->get(feature);

			if (DEBUG) 
				printDebugFeatureInfo(feature, value);

			if (value != 0)
				result += **value;

			feature->deallocate();
		}
	}

	if (DEBUG) {
		std::ostringstream ostr;
		ostr << (boost::format("\t%30s%10d") % "TOTAL:" % result) << std::endl;
		SessionLogger::info("SERIF") << ostr.str();
	}

	return result;
}

double PDecoder::scoreWithPrevTagState(const DTState &state, const DTFeatureType **featureTypes, 
							const int n_feature_types) 
{
	double result = 0;


	if (_use_buffered_features) {
		int tag = _tagSet->getTagIndex(state.getTag());	
		for (int i = 0; i < _n_with_prev_tag_buffered_features; i++) {
			result += _withPrevTagFeatureBuffer[i][tag];
		}
		return result;
	}
	
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	for (int i = 0; i < n_feature_types; i++) {
		int n_features = featureTypes[i]->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			PWeight *value = _weights->get(feature);

			if (DEBUG)
				printDebugFeatureInfo(feature, value);

			if (value != 0)
				result += **value;
		
			feature->deallocate();
		}
	}

	return result;
}

double PDecoder::scoreObservationOnlyState(const DTState &state, const DTFeatureType **featureTypes, 
							const int n_feature_types) 
{
	double result = 0;


	if (_use_buffered_features) {
		int tag = _tagSet->getTagIndex(state.getTag());	
		for (int i = 0; i < _n_observation_only_buffered_features; i++) {
			result += _observationOnlyFeatureBuffer[i][tag];
		}
		return result;
	}
	
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	for (int i = 0; i < n_feature_types; i++) {
		int n_features = featureTypes[i]->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			PWeight *value = _weights->get(feature);

			if (DEBUG)
				printDebugFeatureInfo(feature, value);

			if (value != 0)
				result += **value;

			feature->deallocate();
		}
	}

	return result;
}

void PDecoder::loadWithPrevTagFeatureBuffer(const DTState &state, const DTFeatureType **featureTypes, const int n_feature_types) {
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	_n_with_prev_tag_buffered_features = 0;

	if (!_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::loadWithPrevTagFeatureBuffer()",
				"This method is only compatible with the buffered features setting.");
	}

	for (int i = 0; i < n_feature_types; i++) {
		int n_features = featureTypes[i]->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			_weightsByBlock->load(feature, _withPrevTagFeatureBuffer[_n_with_prev_tag_buffered_features++]);
			feature->deallocate();
		}
	}
}

void PDecoder::loadObservationOnlyFeatureBuffer(const DTState &state, const DTFeatureType **featureTypes, const int n_feature_types) {
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	_n_observation_only_buffered_features = 0;

	if (!_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::loadObservationOnlyFeatureBuffer()",
				"This method is only compatible with the buffered features setting.");
	}

	for (int i = 0; i < n_feature_types; i++) {
		int n_features = featureTypes[i]->extractFeatures(state, featureArray);
		for (int j = 0; j < n_features; j++) {
			DTFeature *feature = featureArray[j];
			_weightsByBlock->load(feature, _observationOnlyFeatureBuffer[_n_observation_only_buffered_features++]);
			feature->deallocate();
		}
	}
}

void PDecoder::setupTagVector(int n_obs, Symbol *tagSyms, int *tags) {
	int start_tag = _tagSet->getStartTagIndex();
	int end_tag = _tagSet->getEndTagIndex();

	tags[0] = start_tag;
	for (int i = 0; i < n_obs; i++) {
		tags[i + 1] = _tagSet->getTagIndex(tagSyms[i]);
	}
	tags[n_obs - 1] = end_tag;
}

void PDecoder::adjustAllWeights(double delta){

	if (_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::adjustAllWeights()",
				"This method is incompatible with the buffered features setting.");
	}

	DTFeature::FeatureWeightMap::iterator curr = _weights->begin();
	DTFeature::FeatureWeightMap::iterator end = _weights->end();
	while(curr != end){
		PWeight* value =(&(*curr).second);
		(**value) = delta;
		value->addToSum();
		++curr;
	}
}

void PDecoder::adjustWeights(std::vector<DTObservation *> & observations,
							 int *tags, double delta,
							 bool add_unseen_features,
							 int n_obs_to_ignore,
							 bool *correct_inds)
{
	int n_obs = static_cast<int>(observations.size());
	if (_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::adjustWeights()",
				"This method is incompatible with the buffered features setting.");
	}

	double adjusted_delta = delta * _n_examples;
	DTFeature *featureArray[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	//MRF: try to match Java DT, the java implementation
	// ignores observations based on START and END observations for namefinding
	for (int i = n_obs_to_ignore; i < (n_obs - n_obs_to_ignore); i++) {
//		if(correct_inds[i]) // only updates where there is error
//			continue;
		int prev_tag = tags[i - 1];
		int tag = tags[i];
		
		const Symbol &rTag = _tagSet->getReducedTagSymbol(tag);
		const Symbol &srTag = _tagSet->getSemiReducedTagSymbol(tag);

		DTState state(_tagSet->getTagSymbol(tag), rTag, srTag,
					  _tagSet->getTagSymbol(prev_tag), i,
					  observations);

		for (int j = 0; j < _featureTypes->getNFeaturesTypes(); j++) {
			int n_features = _featureTypes->getFeatureType(j)
										->extractFeatures(state, featureArray);

			for (int k = 0; k < n_features; k++) {
				DTFeature *feature = featureArray[k];
				PWeight *value = _weights->get(feature);

				if (DEBUG)
					printDebugFeatureInfo(feature, value);

				if (value != 0) {
					**value += delta;
					if (_use_lazy_sum) {
						value->getUpdate() += adjusted_delta;
					}
					// the feature is already in the hash table, so delete this
					// copy of it
					feature->deallocate();
				}
				else {
					// add the feature to the table, and don't delete it
					if (add_unseen_features) {
						PWeight& weight = (*_weights)[feature];
						*weight = delta;
						if (_use_lazy_sum) {
							weight.getUpdate() = adjusted_delta;
						}
					} else {
						feature->deallocate();
					}
				}

			}
		}
	}
}



void PDecoder::tracePath(int n_obs, int *traceback, int *tag_trace) {
	int n_tags = _tagSet->getNTags();
	int j = _tagSet->getEndTagIndex();
	for (int i = n_obs - 1; i >= /* XXX >? */ 0; i--) {
		tag_trace[i] = j;
		j = traceback[n_tags*i + j];
	}
}

/** used to find the second best path */
void PDecoder::tracePathFromMiddle(int n_obs, int *traceback, int * traceforward,
								   int secondbestpos, int secondbesttag, int *tag_trace)
{
	int i, j;
	int n_tags = _tagSet->getNTags();
	// backward
	j = secondbesttag;
//	j = _tagSet->getEndTagIndex();
	for (i = secondbestpos; i < /* XXX >? */ n_obs; i++) {
		tag_trace[i] = j;
		j = traceforward[n_tags*i + j];
	}

	//forward
	j = secondbesttag;
	for (i = secondbestpos; i >= /* XXX >? */ 0; i--) {
		tag_trace[i] = j;
		j = traceback[n_tags*i + j];
	}
}

/*
void PDecoder::traceBest2Paths(int n_obs, DTObservation **observations, int *traceback
					 , double *f_score, bool *f_active,
					 , double *b_score, bool *b_active,
					 int *best_tag_trace, int *second_best_tag_trace
					 double *best_score, double *second_best_score)
{
	int n_tags = _tagSet->getNTags();
	int end_tag_index = _tagSet->getEndTagIndex();
	best_score[0] = f_score[n_tags*(n_obs-1) + end_tag_index];
	second_best_score[0] = 0.0;
	int j = end_tag_index;
	// compute the best tag path
	for (int i = n_obs - 1; i >=  0; i--) {
		tag_trace[i] = j;
		j = traceback[n_tags*i + j];
	}
	s
	for (int i = n_obs - 1; i >  0; i--) {
		int tag = tag_trace[i];
		int prev_index = i-1;
		int n_pred_tags = _tagSet->getNPredecessorTags(tag);
		int *pred_tags = _tagSet->getPredecessorTags(tag);
		for (int p = 0; p < n_pred_tags; p++) {
			int prev_tag = pred_tags[p];
			if(prev_tag==tag_trace[prev_index])
				continue;
			if(!b_active[n_tags*prev_index + prev_tag] || !a_active[n_tags*prev_index + prev_tag]
				continue;
			double score = b_score[n_tags*prev_index + prev_tag] + a_score[n_tags*prev_index + prev_tag];
			

			if ( !b_active[n_tags*prev_index + prev_tag] ||
				score > b_score[n_tags*prev_index + prev_tag])
			{
			}
		}
	}
	}
//	for(int i = 0; i < n_tags && i!=end_tag_index; i++){
//		if(f_score[n_tags*i + j]>second_best_score[0])
//			second_best_score[0] = f_score[n_tags*i + j];
//	}


}
*/

void PDecoder::convertTagsToSymbols(int n_obs, int *tags, Symbol *tagSyms) {
	for (int i = 0; i < n_obs; i++) {
		tagSyms[i] = _tagSet->getTagSymbol(tags[i+1]);
	}
}

void PDecoder::finalizeWeights(){

	if (_use_buffered_features) {
		throw InternalInconsistencyException("PDecoder::finalizedWeights()",
				"This method is incompatible with the buffered features setting.");
	}

	DTFeature::FeatureWeightMap::iterator iter = _weights->begin();
	if (iter == _weights->end())
		return;
	++iter;
	while(iter != _weights->end()){
		*(*iter).second = (*iter).second.getSum();
		++iter;
	}
}

void PDecoder::splitFeatureTypes() {
	delete[] _observationOnlyFeatureTypes;
	delete[] _withPrevTagFeatureTypes;

	_observationOnlyFeatureTypes = 0;
	_withPrevTagFeatureTypes = 0;

	_n_observation_only_feature_types = 0;
	_n_with_prev_tag_feature_types = 0;

	int n_feature_types = _featureTypes->getNFeaturesTypes();
	for (int i = 0; i < n_feature_types; i++) {
		if (_featureTypes->getFeatureType(i)->getInfoSource()->usePrevTag()) {
			_n_with_prev_tag_feature_types++;
		} else {
			_n_observation_only_feature_types++;
		}
	}

	if (_n_observation_only_feature_types > 0)
		_observationOnlyFeatureTypes = _new const DTFeatureType*[_n_observation_only_feature_types];

	if (_n_with_prev_tag_feature_types > 0)
		_withPrevTagFeatureTypes = _new const DTFeatureType*[_n_with_prev_tag_feature_types];

	int count_observation_only_feature_types = 0;
	int count_with_prev_tag_feature_types = 0;

	for (int i = 0; i < n_feature_types; i++) {
		const DTFeatureType *featureType = _featureTypes->getFeatureType(i);
		if (featureType->getInfoSource()->usePrevTag())
			_withPrevTagFeatureTypes[count_with_prev_tag_feature_types++] = featureType;
		else
			_observationOnlyFeatureTypes[count_observation_only_feature_types++] = featureType;
	}
}

bool PDecoder::isAllowableTag(int tag, int constraint_tag) const {
	
	if ((constraint_tag == -1) ||
		(tag == constraint_tag) ||
		(_tagSet->isNoneTag(constraint_tag) && _tagSet->isNoneTag(tag)))
	{
		return true;
	}
	
	return false;
}


void PDecoder::printDebugStateInfo(const DTState &state) const {
	SessionLogger::info("SERIF") << "TAG: " << state.getTag().to_debug_string() << std::endl;
}

/* Prints the given feature and value. Use to debug 
 * which features are being used. By adding the if statement
 * (feature->getFeatureType()->getName() == Symbol(L"<your-feature-name>")
 * you can only look at the features you are interested in.
 */
void PDecoder::printDebugFeatureInfo(DTFeature *feature, PWeight *value) const {

	if (feature == 0)
		return;

	std::stringstream strm;
	std::wstring feature_str;
	feature->toStringWithoutTag(feature_str);

	strm << (boost::format("\t%-15s%-15s") 
		% feature->getFeatureType()->getName().to_debug_string())
		% std::string(feature_str.begin(), feature_str.end());
	
	if (value != 0) 
		strm << boost::format("%10d\n") % **value;
	else 
		strm << boost::format("%10d\n") % 0;

	SessionLogger::info("SERIF") << strm.str();
}

void PDecoder::printTrellis(std::vector<DTObservation *> & observations,
                            int *tags, double* score, bool* active) const 
{
	int n_obs = static_cast<int>(observations.size());
	ostringstream ostr;
	ostr << boost::format("%8s") % "";
	for (int i = 0; i < n_obs; i++) {
		ostr << boost::format("%15s  ") % observations[i]->toString();
	}
	ostr << endl;

	int n_tags = _tagSet->getNTags();
	for (int j = 0; j < n_tags; j++) {
		ostr << (boost::format("%-8s") % _tagSet->getTagSymbol(j).to_debug_string());
		for (int k = 0; k < n_obs; k++) {
			if (active[n_tags*k + j]) {
				ostr << (boost::format("%15d") % score[n_tags*k + j]);
				if (tags != 0 && tags[k] == j) 
					ostr << "**";
				else
					ostr << "  ";
			}
			else { 
				ostr << (boost::format("%15s  ") % "-----");
			}
		}
		ostr << endl;
	}
	SessionLogger::info("SERIF") << ostr.str();
}
