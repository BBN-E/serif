// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_PROB_MODEL_SET_H
#define ET_PROB_MODEL_SET_H

#include "Generic/common/Symbol.h"
#include "Generic/common/ParamReader.h"
#include "Generic/events/stat/EventTriggerFilter.h"
#include "Generic/events/stat/specific_pm_event_models.h"
#include "Generic/discTagger/DTTagSet.h"

#if defined(_WIN32)
	#define swprintf _snwprintf
#endif


class ETProbModelSet {

public:
	ETProbModelSet(DTTagSet *tagSet, double *tagScores, double recall_threshold);
	~ETProbModelSet() { deleteModels(); }

	void clearModels() {
		for (int i = 0; i < _n_models; i++) 
			clearModel(i);
	}	
	void deleteModels() {
		for (int i = 0; i < _n_models; i++) 
			deleteModel(i);
	}	
	void createEmptyModels() {
		for (int i = 0; i < _n_models; i++)
			if (_use_model[i]) createEmptyModel(i);
	}
	void loadModels(const char *model_prefix) {
		for (int i = 0; i < _n_models; i++) 
			if (_use_model[i]) loadModel(i, model_prefix);
	}	
	void deriveModels() {
		for (int i = 0; i < _n_models; i++) 
			if (_use_model[i]) deriveModel(i);
	}
	void printModels(const char *model_prefix) {
		for (int i = 0; i < _n_models; i++) 
			if (_use_model[i]) printModel(i, model_prefix);
	}
	void addTrainingEvent(EventTriggerObservation *observation, Symbol et) {
		for (int i = 0; i < _n_models; i++) 
			if (_use_model[i]) addEvent(observation, et, i);
	}
	void printPMDebugScores(EventTriggerObservation *observation, UTF8OutputStream& out) {
		for (int i = 0; i < _n_models; i++) {
			if (_use_model[i]) {
				//Make 'non-prefered' ie ignored model output grey
				bool model_used = preferModel(observation, i);
				if(!model_used)
					out<<"<FONT color=\"grey\">\n";

				printDebugScores(out,observation, i);
				if(!model_used)
					out<<"</FONT>\n";
			}
		}
	}
	int getAnswer(EventTriggerObservation *observation) {
		for (int i = 0; i < _n_models; i++) {
			if (_use_model[i]) {
				int answer = decodeToDistribution(observation, i);
				if (answer != _tagSet->getNoneTagIndex())
					return answer;
			}
		}
		return _tagSet->getNoneTagIndex();
	}

	enum { WORD, OBJ, SUB, WNET, CLUSTER16, CLUSTER1216, CLUSTER81216, WORD_PRIOR };

	void printDebugScores(UTF8OutputStream& out, EventTriggerObservation *observation, int i);
	bool useWordNet() { return _use_model[WNET]; }
	bool useModel(int mt) { return _use_model[mt]; }
	int getNModels() { return _n_models; }
	int decodeToDistribution(EventTriggerObservation *observation, int mt);
	double getProbability(EventTriggerObservation *observation, Symbol et, int mt);	
	double getLambdaForFullHistory(EventTriggerObservation *observation, Symbol et, int mt) const;
	bool preferModel(EventTriggerObservation *observation, int mt) const;

private:
	type_et_word_model_t *_wordModel;
	type_et_word_prior_model_t *_wordPriorModel;
	type_et_obj_model_t *_objModel;
	type_et_sub_model_t *_subModel;
	type_et_wordnet_model_t *_wordNetModel;
	type_et_cluster16_model_t *_cluster16Model;	
	type_et_cluster1216_model_t *_cluster1216Model;	
	type_et_cluster81216_model_t *_cluster81216Model;
	int _n_models;
	bool _use_model[10];
	double *_tagScores;
	DTTagSet *_tagSet;
	double _recall_threshold;

	void getModelName(int mt, const char *model_prefix, char *buffer);
	void loadModel(int mt, const char *model_prefix);
	void printModel(int mt, const char *model_prefix);
	void createEmptyModel(int mt);
	void clearModel(int mt);
	void deleteModel(int mt);
	void addEvent(EventTriggerObservation *observation, Symbol et, int mt);
	void deriveModel(int mt);
  
};

#endif
