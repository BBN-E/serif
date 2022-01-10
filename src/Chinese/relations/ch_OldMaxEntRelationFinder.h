// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"

#include "Generic/theories/RelationConstants.h"
#include "Generic/theories/RelMention.h"
#include "Chinese/relations/ch_RelationFinder.h"
#include "Generic/CASerif/correctanswers/CorrectDocument.h"

class Proposition;
class Mention;
class Argument;
class PotentialRelationInstance;
class RelationModel;
class PotentialRelationCollector;

class OldMaxEntRelationFinder {
public:
	OldMaxEntRelationFinder();
	~OldMaxEntRelationFinder();
	void cleanup() {}

	void resetForNewSentence();

	RelMentionSet *getRelationTheory(const Parse *parse,
			                       MentionSet *mentionSet,
			                       PropositionSet *propSet,
								   const Parse *secondaryParse = 0);

	static UTF8OutputStream _debugStream;
	static bool DEBUG;

	CorrectDocument *currentCorrectDocument;

private:
	RelationModel *_model;
	PotentialRelationCollector *_collector;

	RelMention *_relations[MAX_SENTENCE_RELATIONS];
	int _n_relations;
	void addRelation(const Mention *first, 
		const Mention *second, int type,
		float score);

	MentionSet *_currentMentionSet;
	int _currentSentenceIndex;

	bool _use_correct_answers;
};
