#ifndef XX_RELATION_MODEL_H
#define XX_RELATION_MODEL_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/relations/RelationModel.h"


class PotentialRelationInstance;

class DefaultRelationModel : public RelationModel {
private:
	friend class DefaultRelationModelFactory;

public:
	int findBestRelationType(PotentialRelationInstance *instance) { return 0; }
	void train(char *training_file, char* output_file_prefix) {};

private:
	DefaultRelationModel() {}
	DefaultRelationModel(const char *file_prefix) {}

};


// RelationModel factory
class DefaultRelationModelFactory: public RelationModel::Factory {
	virtual RelationModel *build() { return _new DefaultRelationModel(); } 
	virtual RelationModel *build(const char *file_prefix) { return _new DefaultRelationModel(file_prefix); } 
};

#endif
