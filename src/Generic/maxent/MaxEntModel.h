// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MAXENT_MODEL_H
#define MAXENT_MODEL_H
#include "Generic/common/DebugStream.h"
#include "Generic/discTagger/DTFeature.h"

class DTTagSet;
class DTFeatureTypeSet;
class DTObservation;
class ObservationInfo;
class DTState;
class MaxEntEventSet;
 
class MatrixBlock {
	public:
		MatrixBlock(int i) { id = i; next = child = 0; }
		int id;
		MatrixBlock *next;
		MatrixBlock *child;
};

class MaxEntModel {
public:
	MaxEntModel(DTTagSet *tagSet, DTFeatureTypeSet *featureTypes,
				   DTFeature::FeatureWeightMap *weights, int train_mode = SCGIS,
				   int percent_held_out = 0, int max_iterations = 1000,
				   double variance = 0, double min_likelihood_delta = .0001, 
				   int stop_check_freq = 1,
				   const char *train_vector_file = 0,
				   const char *held_out_vector_file = 0);
	~MaxEntModel();

	int decodeToInt(DTObservation *observation);
	Symbol decodeToSymbol(DTObservation *observation);
	int decodeToInt(DTObservation *observation, double& score, bool normalize_scores = false);
	Symbol decodeToSymbol(DTObservation *observation, double& score, bool normalize_scores = false);
	int decodeToDistribution(DTObservation *observation, double *scores, 
		int max_scores, int* best_tag = 0);

	void addToTraining(DTObservation *observation, int correct_answer);
	void addToTraining(DTState state, int count = 1);

	void deriveModel(int pruning_threshold, bool const continuous_training = false);

	UTF8OutputStream _debugStream;
	bool DEBUG;

	std::wstring getDebugInfo(DTObservation *observation, int answer);
	void printHTMLDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug);
	void printDebugInfo(DTObservation *observation, int answer, UTF8OutputStream& debug);
	void printDebugInfo(DTObservation *observation, int answer, DebugStream& debug);
	void printDebugTable(DTObservation *observation, UTF8OutputStream& debug);

	enum { GIS, SCGIS };

private:

	DTTagSet *_tagSet;
	DTFeatureTypeSet *_featureTypes;
	DTFeature::FeatureWeightMap *_weights;


	MaxEntEventSet *_observedEvents;
	MaxEntEventSet *_heldOutEvents;
	int _n_events_added;

	DTFeature *_featureBuffer[DTFeatureType::MAX_FEATURES_PER_EXTRACTION];
	
	// training settings
	const int _trainMode;
	const int _percentHeldOut;
	const int _max_iterations;
	const int _stop_check_freq;
	const double _min_likelihood_delta;
	const double _variance;

	int _constantC;
	double _inverseC;
	//static const int LOG_OF_ZERO;
	int _n_observations;
	int _n_features;

	// training data structures
	double *_observed;
	double *_expected;
	double *_logAlphas;
	MatrixBlock **_matrix;
	int *_obsCount;

	// GIS
	double *_s;
	int *_tagCount;
	
	//  SCGIS
	double **_S;
	double *_Z;
	

private:
	void deriveModelGIS(int pruning_threshold, bool const continuous_training = false); 
	void deriveModelSCGIS(int pruning_threshold, bool const continuous_training = false); 

	// WARNING: these functions should only be called during training
	double newtonsMethodGaussianSCGIS(int id);
	double findLogLikelihood(MaxEntEventSet *observations);
	double findPercentCorrect(MaxEntEventSet *observations);
	double findFMeasure(MaxEntEventSet *observations);	
	double scoreStateDuringDerivation(ObservationInfo *obsInfo, int tag);

	double scoreState(const DTState &state);
	
	void printDebugInfo(DTObservation *observation, int answer);
};

#endif
