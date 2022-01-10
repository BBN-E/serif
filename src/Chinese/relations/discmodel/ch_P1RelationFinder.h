#ifndef CH_P1RELATION_FINDER_H
#define CH_P1RELATION_FINDER_H

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
class PropositionSet;
class P1Decoder;
class RelationObservation;
class RelMentionSet;
class RelMention;
class VectorModel;
class DTTagSet;
class SymbolHash;
class PotentialRelationInstance;


class ChineseP1RelationFinder {
public:
	ChineseP1RelationFinder();
	~ChineseP1RelationFinder();
	void cleanup();

	void resetForNewSentence();

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	RelMentionSet *getRelationTheory(EntitySet *entitySet,
		                           const Parse *parse,
			                       MentionSet *mentionSet,
								   ValueMentionSet *valueMentionSet,
			                       PropositionSet *propSet);

private:
	P1Decoder *_decoder;
	VectorModel *_vectorModel;
	DTTagSet *_tagSet;
	RelationObservation *_observation;
	PotentialRelationInstance *_inst;
	int _currentSentenceIndex;
	double _overgen_percentage;

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, const Mention *second, Symbol type);

	SymbolHash *_execTable;
	SymbolHash *_staffTable;

};


#endif
