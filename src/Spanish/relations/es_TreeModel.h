// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef es_TREE_MODEL_H
#define es_TREE_MODEL_H

#include <cstddef>
template <size_t N>
class NgramScoreTableGen;
class Symbol;
class UTF8InputStream;
class PotentialRelationInstance;
class DTRelationSet;
class RelationObservation;
class PropositionSet;
class MentionSet;
class ValueMentionSet;
class Parse;
#include "Spanish/relations/es_specific_relation_tree_models.h"

class TreeModel {
private:
	NgramScoreTable *_originalData;

	type_attachment_model_t *_attachmentModel;
	type_node_model_t *_nodeModel;
	type_predicate_model_t *_predicateModel;
	type_construction_model_t *_constructionModel;
	type_prior_model_t *_priorModel;

	wchar_t _last_prob_buffer[1000];

	/** true if we first decide whether a relation exists,
	  * and then classify it -- false if we make the decision
	  *	in one step */
	bool SPLIT_LEVEL_DECISION;

	RelationObservation *_observation;
	PotentialRelationInstance *_inst;

	void collectFromLoader(class TrainingLoader *trainingLoader);
	void collectFromSentence(const Parse* parse, const MentionSet* mset,		
		const ValueMentionSet *vset,
		PropositionSet *propSet, DTRelationSet *relSet);


public:
	TreeModel();
	TreeModel(const char *file_prefix, bool splitlevel = false);
	~TreeModel();
	void train(char *training_file, char* output_file_prefix);
	void trainFromStateFileList(char *training_file, char* output_file_prefix);

	static Symbol LEFT;
	static Symbol RIGHT;
	static Symbol NESTED;
	static Symbol DUMMY;
	
	int findBestRelationType(PotentialRelationInstance *instance);
	bool hasZeroProbability(PotentialRelationInstance *instance, int type);
	float getProbability(PotentialRelationInstance* instance);
	
	void collect(const char* datafile);
	void printTables(const char* prefix);
	void readInModels(const char* prefix);

	float getPriorProbability(PotentialRelationInstance *inst, Symbol indicator) {
		return (float)_priorModel->getProbability(inst, indicator);
	}
	float getPredicateProbability(PotentialRelationInstance *inst, Symbol indicator) {
		return (float)_predicateModel->getProbability(inst, indicator);
	}
	float getConstructionProbability(PotentialRelationInstance *inst, Symbol indicator) {
		return (float)_constructionModel->getProbability(inst, indicator);
	}
	float getNodeProbability(PotentialRelationInstance *inst, Symbol indicator) {
		return (float)_nodeModel->getProbability(inst, indicator);
	}
	float getAttachmentProbability(PotentialRelationInstance *inst, Symbol indicator) {
		return (float)_attachmentModel->getProbability(inst, indicator);
	}



};

#endif
