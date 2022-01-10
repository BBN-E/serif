// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_POTENTIAL_RELATION_COLLECTOR_H
#define AR_POTENTIAL_RELATION_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/limits.h"
#include "Generic/common/DebugStream.h"
#include "Generic/common/hash_map.h"
#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/relations/PotentialRelationCollector.h"
#include <vector>

class DocTheory;
class TokenSequence;
class Argument;
class Mention;
class MentionSet;
class Parse;
class Proposition;
class PropositionSet;
class PotentialTrainingRelation;
class PotentialRelationInstance;
class RelationTypeMap;


class ArabicPotentialRelationCollector : public PotentialRelationCollector {
private:
	friend class ArabicPotentialRelationCollectorFactory;

public:

	~ArabicPotentialRelationCollector()
	{	finalize(); }

	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);
	virtual void produceOutput(std::wstring *results) { PotentialRelationCollector::produceOutput(results); }
	void outputPacketFile(const char *output_dir, const char *packet_name);

	void collectPotentialDocumentRelations();
	void collectPotentialSentenceRelationsFromChunk(const Parse *parse, MentionSet *mentionSet);
	void collectPotentialSentenceRelationsFromParse(const Parse *parse, MentionSet *mentionSet);


	int getNRelations() { return _n_relations; }
	PotentialTrainingRelation* getPotentialTrainingRelation(int i);
	PotentialRelationInstance* getPotentialRelationInstance(int i); 
	
	int _collectionMode;
	enum { EXTRACT, TRAIN, CORRECT_ANSWERS };

	const RelationTypeMap *_relationTypes;

	int getMode() { return _collectionMode; }

private:

	ArabicPotentialRelationCollector(int collectionMode, const RelationTypeMap *relationTypes = 0);

	void finalize();
	void addPotentialRelation(Mention *ment1, Mention *ment2);

	void outputPotentialTrainingRelations();
	void outputCorrectAnswerFile();
	void outputCorrectEntities();
	void outputCorrectRelations();

	void traverseNode(const SynNode *node); 
	void processMaximalNP(const SynNode *node, std::vector<Mention*> &mentions); 
	
	void processNP(const SynNode *node, int& index);
	void processSubNP(const SynNode *node, std::vector<Mention*> &mentions);

	const wchar_t* convertMentionType(Mention* ment);
	Symbol getDefaultSubtype(EntityType type);

	// the data we need
	DocTheory *_docTheory;
	MentionSet *_mentionSet;

	GrowableArray<std::wstring> _outputFiles;

	GrowableArray<PotentialTrainingRelation *> _trainingRelations;
	GrowableArray<PotentialRelationInstance *> _relationInstances;
	int _n_relations;

	UTF8OutputStream _outputStream;
	DebugStream _debugStream;
	Symbol _primary_parse;

public:
	struct HashKey {
		size_t operator()(const PotentialTrainingRelation r) const {
			return r.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const PotentialTrainingRelation r1, const PotentialTrainingRelation r2) const {
            return (r1) == (r2);
        }
    };
	typedef serif::hash_map<PotentialTrainingRelation, Symbol, HashKey, EqualKey> PotentialRelationMap;

	PotentialRelationMap* _annotationSet;
	PotentialRelationMap* readPacketAnnotation(const char *packet_file);
};

class ArabicPotentialRelationCollectorFactory: public PotentialRelationCollector::Factory {
	virtual PotentialRelationCollector *build(int collectionMode) { return _new ArabicPotentialRelationCollector(collectionMode); }
	virtual PotentialRelationCollector *build(int collectionMode, RelationTypeMap *relationTypes) { return _new ArabicPotentialRelationCollector(collectionMode, relationTypes); }
};


#endif
