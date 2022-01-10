#ifndef es_RELATION_MODEL_H
#define es_RELATION_MODEL_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

class VectorModel;
class TreeModel;
class PatternMatcherModel;
class PotentialRelationInstance;
#include "Generic/theories/RelationConstants.h"
#include "Generic/relations/RelationModel.h"


class SpanishRelationModel : public RelationModel {
private:
	friend class SpanishRelationModelFactory;

	VectorModel* _vectorModel;
	TreeModel* _treeModel;
	PatternMatcherModel* _patternModel;
	bool USE_PATTERNS;
	bool USE_METONYMY;
	
public:
	int findBestRelationType(PotentialRelationInstance *instance);
	int findBestRelationTypeLeftMetonymy(PotentialRelationInstance *instance);
	int findBestRelationTypeRightMetonymy(PotentialRelationInstance *instance);
	int findBestRelationTypeBasic(PotentialRelationInstance *instance);
	void train(char *training_file, char* output_file_prefix);
	void trainFromStateFileList(char *training_file, char* output_file_prefix);

	static Symbol forcedOrgSym;
	static Symbol forcedPerSym;

private:
	SpanishRelationModel();
	SpanishRelationModel(const char *file_prefix);

};


// RelationModel factory
class SpanishRelationModelFactory: public RelationModel::Factory {
	virtual RelationModel *build() { return _new SpanishRelationModel(); } 
	virtual RelationModel *build(const char *file_prefix) { return _new SpanishRelationModel(file_prefix); } 
};

#endif
