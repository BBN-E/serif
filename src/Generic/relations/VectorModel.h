// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef VECTOR_MODEL_H
#define VECTOR_MODEL_H

#include <cstddef>
template<size_t N>
class NgramScoreTableGen;
class Symbol;
class UTF8InputStream;
class DTRelationSet;
class RelationObservation;
class PropositionSet;
class MentionSet;
class ValueMentionSet;
class Parse;
class PotentialRelationInstance;
#include "Generic/relations/specific_relation_vector_models.h"


class VectorModel {
private:
	NgramScoreTable *_originalData;
	type_feature_vector_model_t *_model;
	type_b2p_feature_vector_model_t *_b2pModel;

	/** true if we first decide whether a relation exists,
	  * and then classify it -- false if we make the decision
	  *	in one step */
	bool SPLIT_LEVEL_DECISION;
	RelationObservation *_observation;
	PotentialRelationInstance *_inst;
	void trainFromLoader(class TrainingLoader *trainingLoader);
	void trainFromSentence(const Parse* parse, const MentionSet* mset,
		const ValueMentionSet *vset,
		PropositionSet *propSet, DTRelationSet *relSet);
	int countSentencesInFile(const wchar_t *filename);


public:
	VectorModel();
	VectorModel(const char *file_prefix, bool splitlevel = false);
	~VectorModel();
	void train(char *training_file, char* output_file_prefix);
	void trainFromStateFileList(char *training_file, char* output_file_prefix);

	bool hasZeroProbability(PotentialRelationInstance *instance, int type);
	int findBestRelationType(PotentialRelationInstance *instance);
	int findConfidentRelationType(PotentialRelationInstance *instance, 
		double lambda_threshold = .5);
	float lookup(PotentialRelationInstance *instance) const;
	float lookupB2P(PotentialRelationInstance *instance) const;

};

#endif
