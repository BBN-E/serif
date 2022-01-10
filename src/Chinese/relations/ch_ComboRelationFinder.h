#ifndef CH_COMBO_RELATION_FINDER_H
#define CH_COMBO_RELATION_FINDER_H

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
class MaxEntModel;
class PotentialRelationInstance;
class SymbolHash;
#include "Generic/discTagger/DTFeature.h"

class ChineseComboRelationFinder {
public:
	ChineseComboRelationFinder();
	~ChineseComboRelationFinder();
	void cleanup() {}

	void resetForNewSentence();

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	RelMentionSet *getRelationTheory(EntitySet *entitySet,
								   const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet);

	void allowMentionSetChanges() { _allow_mention_set_changes = true; }
	void disallowMentionSetChanges() { _allow_mention_set_changes = false; }

private:

	P1Decoder *_p1Decoder;
	P1Decoder *_p1SecondaryDecoder;
	MaxEntModel *_maxentDecoder;
	VectorModel *_vectorModel;
	DTTagSet *_tagSet;
	DTFeature::FeatureWeightMap *_p1Weights;
	DTFeature::FeatureWeightMap *_p1SecondaryWeights;
	DTFeature::FeatureWeightMap *_maxentWeights;
	class DTFeatureTypeSet *_featureTypes;
	class DTFeatureTypeSet *_p1SecondaryFeatureTypes;
	RelationObservation *_observation;
	PotentialRelationInstance *_inst;
	int _currentSentenceIndex;

	bool fixRelation(const Mention *first, const Mention *second, Symbol type);

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, const Mention *second, Symbol type);
	void findMentionSetChanges(MentionSet *mentionSet);
	void tryForcingRelations(MentionSet *mentionSet);
	bool shouldBeReversed(Symbol answer);


	Symbol findSpecialCaseRelation(MentionSet *mentionSet);
	static Symbol NO_RELATION_SYM;

	bool _allow_mention_set_changes;

	SymbolHash *_execTable;
	SymbolHash *_staffTable;
	void loadSymbolHash(SymbolHash *hash, const char* file);

};


#endif
