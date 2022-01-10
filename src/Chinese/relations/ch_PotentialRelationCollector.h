// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CH_POTENTIAL_RELATION_COLLECTOR_H
#define CH_POTENTIAL_RELATION_COLLECTOR_H

#include "Generic/results/ResultCollector.h"
#include "Generic/common/GrowableArray.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/limits.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/AutoGrowVector.h"
#include "Generic/relations/PotentialTrainingRelation.h"
#include "Generic/relations/PotentialRelationCollector.h"

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


class ChinesePotentialRelationCollector : public PotentialRelationCollector {
private:
	friend class ChinesePotentialRelationCollectorFactory;

public:

	~ChinesePotentialRelationCollector();

	virtual void resetForNewSentence();
	virtual void loadDocTheory(DocTheory *docTheory);

	virtual void produceOutput(const wchar_t *output_dir,
							   const wchar_t *document_filename);
	virtual void produceOutput(std::wstring *results) { PotentialRelationCollector::produceOutput(results); }
	void outputPacketFile(const char *output_dir, const char *packet_name);

	void collectPotentialDocumentRelations();
	void collectPotentialSentenceRelations(const Parse *parse, MentionSet *mentionSet, 
									       const PropositionSet *propSet);

	int getNRelations() { return _n_relations; }
	PotentialTrainingRelation* getPotentialTrainingRelation(int i);
	PotentialRelationInstance* getPotentialRelationInstance(int i);
	
	const RelationTypeMap *_relationTypes;

	int _collectionMode;
	enum { EXTRACT, TRAIN, CLASSIFY, CORRECT_ANSWERS };

private:

	ChinesePotentialRelationCollector(int collectionMode, RelationTypeMap *relationTypes = 0);

	void finalize();

	void outputPotentialTrainingRelations();
	void outputCorrectAnswerFile();
	void outputCorrectEntities();
	void outputCorrectRelations();

	void classifyCopulaProposition(Proposition *prop);
	void classifySetProposition(Proposition *prop);
	void classifyCompProposition(Proposition *prop);
	void classifyProposition(Proposition *prop);
	void findListInstances(Argument *left, Argument *right, Proposition *prop, bool leftIsList);
	void findNestedInstances(Argument *first, Argument *intermediate, Proposition *prop); 
	void classifyArgumentPair(Argument *leftArg, Argument *rightArg, Proposition *prop);
	void classifyPropArgumentPair(Argument *left, Argument *right, Proposition *prop, bool leftIsProp);


	void addPotentialRelation(Argument *arg1, Argument *arg2, const Proposition *prop);
	void addNestedPotentialRelation(Argument *arg1, Argument *intermediate, Argument *arg2, 
									const Proposition *outer_prop, const Proposition *inner_prop); 
	void addMetonymicRelations(Argument *arg1, Argument *arg2, const Proposition *prop);
	void addMetonymicNestedRelations(Argument *arg1, Argument *intermediate, Argument *arg2, 
									  const Proposition *outer_prop, const Proposition *inner_prop);
	void distributeOverSet(Proposition *set, Argument *arg, bool set_is_arg1, const Proposition *prop);
	void distributeOverNestedSet(Proposition *set, Argument *arg, Argument *intermediate,
								 bool set_is_arg1, const Proposition *outer_prop, const Proposition *inner_prop);

	void collectDefinitions(const PropositionSet *propSet);

	const wchar_t* convertMentionType(Mention* ment);
	Symbol getDefaultSubtype(EntityType type);

	// the data we need
	DocTheory *_docTheory;
	MentionSet *_mentionSet;
	
	AutoGrowVector<Proposition*> _definitions;
	std::vector<Argument *> _propTakesPlaceAt;

	GrowableArray<PotentialTrainingRelation *> _trainingRelations;
	GrowableArray<PotentialRelationInstance *> _relationInstances;
	int _n_relations;

	GrowableArray<std::wstring> _outputFiles;

	UTF8OutputStream _outputStream;

public:
	struct HashKey {
		size_t operator()(const PotentialTrainingRelation& r) const {
			return r.hash_code();
		}
	};
    struct EqualKey {
        bool operator()(const PotentialTrainingRelation& r1, const PotentialTrainingRelation& r2) const {
            return (r1) == (r2);
        }
    };
	typedef serif::hash_map<PotentialTrainingRelation, Symbol, HashKey, EqualKey> PotentialRelationMap;
	typedef serif::hash_map<PotentialTrainingRelation, bool, HashKey, EqualKey> PotentialRelationSet;
	
	PotentialRelationSet _relationSet;
	PotentialRelationMap* _annotationSet;
	PotentialRelationMap* readPacketAnnotation(const char *packet_file);
};

class ChinesePotentialRelationCollectorFactory: public PotentialRelationCollector::Factory {
	virtual PotentialRelationCollector *build(int collectionMode) { return _new ChinesePotentialRelationCollector(collectionMode); }
	virtual PotentialRelationCollector *build(int collectionMode, RelationTypeMap *relationTypes) { return _new ChinesePotentialRelationCollector(collectionMode, relationTypes); }
};


#endif
