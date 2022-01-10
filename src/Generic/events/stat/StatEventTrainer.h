// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STAT_EVENT_TRAINER_H
#define STAT_EVENT_TRAINER_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/hash_map.h"

class EventTriggerFinder;
class EventArgumentFinder;
class EventModalityClassifier;
class LocatedString;

#include <stdio.h>

class StatEventTrainer {
public:
	StatEventTrainer(int mode_);
	~StatEventTrainer();

	void train();
	void roundRobin();
	void selectAnnotation();
	void devTest();
	void eventAADevTestUsingGoldEVMentions();

	enum { TRAIN, ROUNDROBIN, SELECT_ANNOTATION, DEVTEST, AADEVTEST }; // apparently DEVTEST is for event_modality
	const int MODE;

	static Symbol SENT_TAG_SYM;
	static Symbol EVENT_TAG_SYM;

private:
	EventTriggerFinder *_triggerFinder;
	EventArgumentFinder *_argumentFinder;
	EventModalityClassifier *_modalityClassifier;

	bool TRAIN_TRIGGERS;
	bool TRAIN_ARGS;
	bool TRAIN_MODALITY;
	bool DEVTEST_MODALITY;
	bool TWO_LAYER_MODALITY;

	typedef serif::hash_map<Symbol, int, Symbol::Hash, Symbol::Eq> SentIdMap;
	SentIdMap _sentIdMap;

	class Symbol *_docIds;
	class Symbol *_documentTopics;
	class TokenSequence **_tokenSequences;
	class Parse **_parses;
	class Parse **_secondaryParses;
	class NPChunkTheory** _npChunks;
	class ValueMentionSet ** _valueMentionSets;
	class MentionSet ** _mentionSets;
	class PropositionSet ** _propSets;
	class EventMentionSet ** _eventMentionSets;

	class MorphologicalAnalyzer *_morphAnalysis;

	int loadTrainingData(class TrainingLoader *trainingLoader, int start_index = 0);
	int loadAllSelectedAnnotation(const char *file, int start_index, int end_index);
	int loadSelectedAnnotationFile(const wchar_t *file);
	EventMentionSet* processTriggerAnnotationSentence(LocatedString *sent, int sent_no,
										class Parse *parse, class PropositionSet *propSet);
};


#endif
