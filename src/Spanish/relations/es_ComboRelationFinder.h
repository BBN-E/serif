#ifndef es_COMBO_RELATION_FINDER_H
#define es_COMBO_RELATION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"

class Parse;
class Mention;
class MentionSet;
class ValueMentionSet;
class EntitySet;
class EntityType;
class PropositionSet;
class P1Decoder;
class RelationObservation;
class RelMentionSet;
class RelMention;
class DTTagSet;
class VectorModel;
class PatternMatcherModel;
class TreeModel;
class MaxEntModel;
class PotentialRelationInstance;
class SymbolHash;
class PropTreeLinks;
class SentenceTheory;
#include "Generic/discTagger/DTFeature.h"

class SpanishComboRelationFinder {
public:
	SpanishComboRelationFinder();
	~SpanishComboRelationFinder();
	void cleanup();

	void resetForNewSentence();

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	RelMentionSet *getRelationTheory(EntitySet *entitySet,
		SentenceTheory *sentTheory,
								   const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet,
								   const PropTreeLinks* ptLinks=0);

	void allowMentionSetChanges() { _allow_mention_set_changes = true; }
	void disallowMentionSetChanges() { _allow_mention_set_changes = false; }

private:

	P1Decoder *_p1Decoder;
	P1Decoder *_p1SecondaryDecoder;
	MaxEntModel *_maxentDecoder;
	double *_maxentTagScores;
	VectorModel *_vectorModel;
	TreeModel *_treeModel;
	DTTagSet *_tagSet;
	DTTagSet *_maxentTagSet;
	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_p1SecondaryWeights;
	DTFeature::FeatureWeightMap *_maxentWeights;
	class DTFeatureTypeSet *_featureTypes;
	class DTFeatureTypeSet *_p1SecondaryFeatureTypes;
	RelationObservation *_observation;
	PotentialRelationInstance *_inst;
	int _currentSentenceIndex;

	SymbolHash *_facOrgWords;
	void loadSymbolHash(SymbolHash *hash, const char* file);
	Symbol fixEthnicityEtcRelation(const Mention *first, const Mention *second);

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, const Mention *second, Symbol type, float score);
	void findMentionSetChanges(MentionSet *mentionSet);
	void tryForcingRelations(MentionSet *mentionSet);

	/** Return a float indicating whether the given answer should probably have its order
	  * reversed.  If this score is greater than 0, then switch the two relation args. */
	float shouldBeReversedScore(Symbol answer);
	bool shouldBeReversed(Symbol answer);

	bool tryFakeTypeRelation(MentionSet *mentionSet, 
		const Mention *possMention,
		EntityType possibleType, Symbol *acceptableTypes);
	//bool tryCoercedTypeRelation(MentionSet *mentionSet, EntitySet *entitySet,
	//	const Mention *possMention,
	//	EntityType possibleType);

	bool _skip_special_case_answer;
	Symbol findSpecialCaseRelation(MentionSet *mentionSet);
	static Symbol NO_RELATION_SYM;

	bool _allow_mention_set_changes;

	double calculateConfidenceScore(const Mention* mention1, const Mention* mention2, float reversed_score,
									Symbol answer, RelationObservation *obs);

	bool _use_correct_answers;
};


#endif

