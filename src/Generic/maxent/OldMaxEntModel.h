// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OLD_MAXENT_MODEL_H
#define OLD_MAXENT_MODEL_H
#include "Generic/common/DebugStream.h"

class OldMaxEntEvent;
class OldMaxEntEventSet;
class OldMaxEntFeatureTable;
class EventContext;
class UTF8InputStream;

class OldMaxEntModel {
public:
	enum { GIS, IIS, IIS_GAUSSIAN, IIS_FEATURE_SELECTION, GIS_EXPONENTIAL };
	enum { PROBS_CONVERGE, HELD_OUT_LIKELIHOOD };

	// read in a model file
	OldMaxEntModel(const char *prefix);
	// read in model file from open stream
	OldMaxEntModel(UTF8InputStream &stream);
	// create empty tables
	OldMaxEntModel(int n_outcomes, Symbol *outcomes, int mode = GIS, 
			    int stop_criterion = PROBS_CONVERGE, int percent_held_out = 10, 
				double variance = 1.0, int n_features_to_add = 1);

	~OldMaxEntModel();

	// query a trained model for log probability
	double getProbability(OldMaxEntEvent *event) const;
	double getScore(OldMaxEntEvent *event) const;

	double getScoreAndDebug(OldMaxEntEvent *event, DebugStream& debug) const;
	double getScoreAndDebug(OldMaxEntEvent *event, UTF8OutputStream& debug) const;
	
	// queue up data for pending derivation
	void addEvent(OldMaxEntEvent *event) { addEvent(event, 1); }
	void addEvent(OldMaxEntEvent *event, int count); 

	// derive a model from observed events
	void deriveModel(int pruning_threshold = 1, double threshold = 0.001);
	// print the derived model
	void printModel(const char *prefix);
	// print to an already opened stream
	void print_to_open_stream(UTF8OutputStream &stream);

private:
	OldMaxEntFeatureTable *_alphaTable;
	double _correctionAlpha;
	int _constantC;
	double _inverseC;
	int _n_events_added;

	// training settings
	const int _trainMode;
	const int _stopCriterion;
	const int _percentHeldOut;
	const double _variance;
	const int _n_features_to_add;

	static const int LOG_OF_ZERO;

	Symbol *_outcomes;
	int _n_outcomes;

	static DebugStream _debug;

	// data structures for training
	OldMaxEntEventSet *_observedEvents;
	double *_observedContextProbs;
	double **_observedEventProbs;
	double **_predictedProbs;
	double **_lastPredictedProbs;
	EventContext **_contexts;
	int ***_featureIDs;
	int **_predicateCounts;
	double *_logAlphas;
	double *_observedFeatureProbs;
	double *_predictedExpectation;
	int *_featureOutcomes;
	int *_activeFeature;

	// used to compute stats about held out events
	OldMaxEntEventSet *_heldOutEvents;
	double **_heldOutObservedEventProbs;
	double **_heldOutPredictedProbs;
	EventContext **_heldOutContexts;
	int ***_heldOutFeatureIDs;
	int **_heldOutPredicateCounts;
	

	// utility methods used by deriveModel
	void calculateObservedProbs();
	void initializeAlphas();
	void findAlphas(int iter);
	void findAlphasGIS(int iter);
	void findAlphasIIS(int iter);
	double findHeldOutLogLikelihood();
	double findHeldOutLogLikelihood(bool &not_converged, double threshold, double &last_likelihood);
	double findTrainingLogLikelihood();
	void findPredictedProbs(bool &not_converged, double threshold);
	void findExpectations();
	double calculateAlphaGIS(double oldAlpha, double obsE, double predE);
	double calculateAlphaIIS(double oldAlpha, double obsE, int id);
	double newtonsMethod(double obsE, int id);
	double newtonsMethodGaussian(double obsE, int id);
	void buildAlphaTable();
	int findMaxFeatureGain(int iter, double threshold, bool &converged);
	int findMaxFeatureGain(int iter, double threshold, bool &converged, int *features, int max_features);
	double calculateGain(int feature_n, double alpha); 

};

#endif
