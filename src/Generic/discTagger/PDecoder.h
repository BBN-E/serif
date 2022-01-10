// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef P_DECODER_H
#define P_DECODER_H

#include "Generic/common/Symbol.h"
#include "Generic/discTagger/DTFeature.h"

class DTTagSet;
class DTObservation;
class DTFeatureTypeSet;
class DTState;
class BlockFeatureTable;

/** Note: the decoding speed has been improved by separating the feature types
  * that do and do not extract features from the previous tag.  This, however, is
  * implemented only on forwardPass(...).  The similar fix can/may be applied to others
  * decoding routines, i.e. backwardPass, constrainedForwardPass, constrainedBackwardPass,
  * but has not yet been implemented.
  */

class PDecoder {
private:
	bool _add_hyp_features;
	
	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	DTFeature::FeatureWeightMap *_weights;

	/**  These are used to improve the decoding speed by caching values for features
	  *  which are identical except for the outcome/tag.  Avoids the need to look up each
	  *  individual feature in the hash table.  Note that this optimization may only be
	  *  used for decoding.
	  */
	bool _use_buffered_features;
	BlockFeatureTable *_weightsByBlock;
	int _max_buffered_features;
	int _n_buffered_tags;
	double** _observationOnlyFeatureBuffer;
	double** _withPrevTagFeatureBuffer;
	int _n_observation_only_buffered_features;
	int _n_with_prev_tag_buffered_features;

	/** These are used to improve the decoding speed by separating the feature types
	  * that do and do not extract features from the previous tag.
	  */
	const DTFeatureType **_observationOnlyFeatureTypes;
	const DTFeatureType **_withPrevTagFeatureTypes;
	int _n_observation_only_feature_types;
	int _n_with_prev_tag_feature_types;

	bool _use_lazy_sum;
	long _n_examples;

	const static bool DEBUG = false;

public:
	/** If add_hyp_features is true, then all wrong-hypothesis features,
	  * even those not seen in training data, will be added to the
	  * weight table. This is different from Scott's QuickTagger, 
	  * seems to improve performance, and takes longer.
	  * For decoding, add_hyp_features has no effect.
	  */
	PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
			 DTFeature::FeatureWeightMap *weights,
			 bool add_hyp_features = true, bool compute_lazy_sum = false);
	PDecoder(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
			 BlockFeatureTable *weights,
			 bool add_hyp_features = true, bool compute_lazy_sum = false);
	~PDecoder();

	/** Helpers for allocating feature buffers */
	void initFeatureBuffers();
	void freeFeatureBuffers();
    
	/** Decode sentence to tag Symbols */
	void decode(std::vector<DTObservation *> & observations,
				Symbol *tags);
	/** Decode sentence to tag index ints */
	double decode(std::vector<DTObservation *> & observations,
				int *tags);
	/** Decode sentence to tag index ints with constraints */
	double constrainedDecode(std::vector<DTObservation *> & observations, int* constraints,
				int *tags);

	/** Add features that occur in given training data to weight table
	  * (with weight of 0) */
	void addFeatures(std::vector<DTObservation *> & observations, int *answers, int n_obs_to_ignore = 0);

	bool train(std::vector<DTObservation *> & observations, 
					 int *answer, int n_obs_to_ignore);

	double trainWithMargin(std::vector<DTObservation *> & observations, 
					 int *answer, int n_obs_to_ignore, double margin);
	
	/**	Decode sentence for active learning tags contains the symbol tags of 
		the best path (same as decode).  Return the MinMargin for the sentence.
		MinMargin is the smallest difference between the best and second best 
		score for the top 2 tags at any point in the sentence.  Scores in this case are 
		the sum of forward and backward sums for every point in the sentence. 
	*/
	double decodeAllTags(std::vector<DTObservation *> & observations, int n_obs_to_ignore,
                             int* best_tags, int* secondbest_tags,
                             int& best_position, double& best_score, double& secondbest_score);
	double decodeAllTags(std::vector<DTObservation *> & observations, 
                             int n_obs_to_ignore = 0, int* tag_ints = 0);
	double decodeAllTagsPos(std::vector<DTObservation *> & observations,  int& bestposition);
	double decodeAllTags(std::vector<DTObservation *> & observations, 
                             int n_obs_to_ignore, int *tag_ints, int *secondbest_tag_ints, 
                             double& best_score, double& secondbest_score);
	double decodeAllTags(std::vector<DTObservation *> & observations,  
                             double* scores, int nscores, 
                             int n_obs_to_ignore = 0, int* tag_ints = 0);

	double constrainedDecodeAllTags(std::vector<DTObservation *> & observations,  int* constraints, int* tag_ints =0);
	
	void adjustAllWeights(double delta);
	void finalizeWeights();

	long getNHypotheses() const { return _n_examples + 1; }

private:

	void forwardPass(std::vector<DTObservation *> & observations, int *traceback);
	void forwardPass(std::vector<DTObservation*> & observations, 
	                 int* traceback, double* f_score, bool* f_active, int* constraints = 0);

	void backwardPass(std::vector<DTObservation *> & observations);
	void backwardPass(std::vector<DTObservation *> & observations, 
	                  int* traceforward, double* b_score, bool* b_active, int* constraints = 0);

	/** original (non-optimized) score state method **/
	double scoreState(const DTState &state);

	/** score state using only the feature types listed in the featureTypes array */
	double scoreObservationOnlyState(const DTState &state, const DTFeatureType **featureTypes, const int n_feature_types);
	double scoreWithPrevTagState(const DTState &state, const DTFeatureType **featureTypes, const int n_feature_types);

	void setupTagVector(int n_obs, Symbol *tagSyms, int *tags);

	void adjustWeights(std::vector<DTObservation *> & observations,
					   int *tags, double delta, bool add_unseen_features,
					   int n_obs_to_ignore = 0, bool *correct_inds = 0);

	/** compute the score of a sequence of tags given a sentance observations */
	double scorePath(std::vector<DTObservation *> & observations, int *answer);

	void tracePath(int n_obs, int *traceback, int *tag_trace);
	/** trace a path from the middle of the matrix.  used to find the second best path. */
	void tracePathFromMiddle(int n_obs, int *traceback, int * traceforward,
						   int secondbestpos, int secondbesttag, int *tag_trace);

	void convertTagsToSymbols(int n_obs, int *tags, Symbol *tagSyms);

	/** separating the feature types that do and do not extract features from the previous tag. */
	void splitFeatureTypes();

	void loadFeatureBuffer(const DTState &state);
	void loadObservationOnlyFeatureBuffer(const DTState &state, 
		const DTFeatureType **featureTypes, const int n_feature_types);
	void loadWithPrevTagFeatureBuffer(const DTState &state, 
		const DTFeatureType **featureTypes, const int n_feature_types);

	bool isAllowableTag(int tag, int constraint_tag) const;
	void printDebugStateInfo(const DTState &state) const;
	void printDebugFeatureInfo(DTFeature *feature, PWeight *value) const;
	void printTrellis(std::vector<DTObservation *> & observations,  int *tags, double* score, bool* active) const;
};

#endif
