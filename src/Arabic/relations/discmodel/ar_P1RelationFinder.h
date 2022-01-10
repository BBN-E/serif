#ifndef AR_P1RELATION_FINDER_H
#define AR_P1RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/discTagger/DTFeature.h"
class Parse;
class Mention;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class PropositionSet;
class P1Decoder;
class RelationObservation;
class RelMentionSet;
class RelMention;
class DTTagSet;
class EntityType;
class Entity;
class SynNode;
class PotentialRelationInstance;
class VectorModel;
class SymbolHash;
class MaxEntModel;
class DTFeatureTypeSet;


class ArabicP1RelationFinder {
public:
	ArabicP1RelationFinder();
	~ArabicP1RelationFinder();

	void resetForNewSentence();
	void cleanup();

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	RelMentionSet *getRelationTheory(EntitySet *entitySet,
								   const Parse *parse,
			                       MentionSet *mentionSet,
								   const ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet,
								   const Parse* secondaryParse);

private:

	P1Decoder *_decoder;
	DTTagSet *_tagSet;
	RelationObservation *_observation;
	int _currentSentenceIndex;
	double _overgen_percentage;

	Symbol _allowPronTypeChange; //none, all, pron-ment
	Symbol _reverseRelations;	//none, both, rule, model
	bool _doGenderMatch;


	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, const Mention *second, Symbol type);
	Symbol findPronounRelation(Mention *first, Mention *second, EntityType currType1,
											 EntityType currType2, EntityType& firstType, 
											 EntityType& secondType);
//	void linkPronounToEntity(Mention* ment, Entity* thisEnt, Entity* otherEnt, 
//		EntitySet* entitySet, MentionSet* mentSet);
	bool isValidRelationEntityTypeCombo(Symbol relationType, 
		EntityType ment1Type, EntityType ment2Type);
	Symbol _decodePronounRelation(Mention* ment1, Mention* ment2, double& score);
	//bool _pronounAndNounAgree(Mention* ment, const SynNode* pron, const SynNode* np);
	bool _reverseRelation(Symbol relationtype, RelationObservation* obs);

	//for argument order choices
	PotentialRelationInstance *_inst;
	VectorModel *_vectorModel;

	//for pronoun linking choices
	SymbolHash* _mascSing;
	//SymbolHash* _mascPl;
	SymbolHash* _femSing;
//	SymbolHash* _femPl;
	void loadSymbolHash(SymbolHash *hash, const char* file);

	SymbolHash *_execTable;
	SymbolHash *_staffTable;

	MaxEntModel *_maxentDecoder;
	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_maxentWeights;
	DTFeatureTypeSet *_featureTypes;


};


#endif
