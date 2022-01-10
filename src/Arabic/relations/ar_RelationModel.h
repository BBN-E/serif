#ifndef AR_RELATION_MODEL_H
#define AR_RELATION_MODEL_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved

#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/theories/RelationConstants.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/relations/RelationModel.h"

class PotentialRelationInstance;
class PotentialRelationCollector;
class Symbol;
class MaxEntRelationModel;
class VectorModel;


class ArabicRelationModel : public RelationModel {
	friend class ArabicRelationModelFactory;

private:
	MaxEntRelationModel* _maxEntModel;

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
	~ArabicRelationModel();

	int findBestRelationType(PotentialRelationInstance *instance) { return 0; };
	void generateVectorsAndTrain(char *packet_file, char* vector_file, char* output_file_prefix);
	void train(char *training_file, char* output_file_prefix) {};
	void trainFromStateFileList(char *training_file, char* output_file_prefix);
	void test(char *test_vector_file) {}; 

private:
	ArabicRelationModel();
	ArabicRelationModel(const char *file_prefix);

	PotentialRelationMap* readPacketAnnotation(const char *packet_file);
	void extractTrainingRelations(PotentialRelationCollector &results);
	VectorModel *_vectorModel;

	DebugStream _debugStream;

};

// RelationModel factory
class ArabicRelationModelFactory: public RelationModel::Factory {
	virtual RelationModel *build() { return _new ArabicRelationModel(); } 
	virtual RelationModel *build(const char *file_prefix) { return _new ArabicRelationModel(file_prefix); } 
};

#endif
