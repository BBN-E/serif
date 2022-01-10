#ifndef CH_RELATION_MODEL_H
#define CH_RELATION_MODEL_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/hash_map.h"
#include "Generic/relations/RelationModel.h"

class PotentialRelationInstance;
class PotentialRelationCollector;
class Symbol;
class VectorModel;
class OldMaxEntRelationModel;


class ChineseRelationModel : public RelationModel {
private:
	friend class ChineseRelationModelFactory;

	VectorModel* _vectorModel;
	OldMaxEntRelationModel* _maxEntModel;

	struct HashKey {
		size_t operator()(const PotentialTrainingRelation& r) const {
			return r.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const PotentialTrainingRelation& r1, const PotentialTrainingRelation& r2) const {
            return r1 == r2;
        }
    };
	typedef serif::hash_map<PotentialTrainingRelation, Symbol, HashKey, EqualKey> PotentialRelationMap;
	
public:
	~ChineseRelationModel();
	int findBestRelationType(PotentialRelationInstance *instance);
	void generateVectorsAndTrain(char *packet_file, char* vector_file, char* output_file_prefix);
	void train(char *training_file, char* output_file_prefix);
	void trainFromStateFileList(char *training_file, char* output_file_prefix);
	void test(char *test_vector_file); 

private:
	ChineseRelationModel();
	ChineseRelationModel(const char *file_prefix);

	PotentialRelationMap* readPacketAnnotation(const char *packet_file);
	void extractTrainingRelations(PotentialRelationCollector &results);

};

// RelationModel factory
class ChineseRelationModelFactory: public RelationModel::Factory {
	virtual RelationModel *build() { return _new ChineseRelationModel(); } 
	virtual RelationModel *build(const char *file_prefix) { return _new ChineseRelationModel(file_prefix); } 
};

#endif
