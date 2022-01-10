#ifndef EN_P1RELATION_FINDER_H
#define EN_P1RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/Limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"

class Parse;
class Mention;
class MentionSet;
class EntitySet;
class EntityType;
class PropositionSet;
class P1Decoder;
class RelationObservation;
class RelMentionSet;
class RelMention;
class DTTagSet;
class VectorModel;
class TreeModel;
class PotentialRelationInstance;
class SymbolHash;
#include "Generic/discTagger/DTFeature.h"

class EnglishP1RelationFinder {
public:
	EnglishP1RelationFinder();
	~EnglishP1RelationFinder();

	void resetForNewSentence();

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	RelMentionSet *getRelationTheory(const Parse *parse,
			                       MentionSet *mentionSet,
			                       PropositionSet *propSet);

private:

	P1Decoder *_decoder;
	VectorModel *_vectorModel;
	TreeModel *_treeModel;
	DTTagSet *_tagSet;
	DTFeature::FeatureWeightMap *_weights;
	class DTFeatureTypeSet *_featureTypes;
	RelationObservation *_observation;
	PotentialRelationInstance *_inst;
	int _currentSentenceIndex;
	double _overgen_percentage;

	SymbolHash *_facOrgWords;
	void loadSymbolHash(SymbolHash *hash, const char* file);
	bool fixRelation(const Mention *first, const Mention *second, Symbol type);

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, const Mention *second, Symbol type);
	void tryForcingRelations(MentionSet *mentionSet);
	bool shouldBeReversed(Symbol answer);
	bool tryFakeTypeRelation(MentionSet *mentionSet, 
		const Mention *possMention,
		EntityType possibleType, Symbol *acceptableTypes);
	//bool tryCoercedTypeRelation(MentionSet *mentionSet, EntitySet *entitySet,
	//	const Mention *possMention,
	//	EntityType possibleType);


};


#endif
