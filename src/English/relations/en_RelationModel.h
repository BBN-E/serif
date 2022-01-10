#ifndef EN_RELATION_MODEL_H
#define EN_RELATION_MODEL_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

class VectorModel;
class TreeModel;
class PatternMatcherModel;
class PotentialRelationInstance;
#include "Generic/theories/RelationConstants.h"
#include "Generic/relations/RelationModel.h"


class EnglishRelationModel : public RelationModel {
private:
	friend class EnglishRelationModelFactory;

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
	EnglishRelationModel();
	EnglishRelationModel(const char *file_prefix);

};


// RelationModel factory
class EnglishRelationModelFactory: public RelationModel::Factory {
	virtual RelationModel *build() { return _new EnglishRelationModel(); } 
	virtual RelationModel *build(const char *file_prefix) { return _new EnglishRelationModel(file_prefix); } 
};

#endif
