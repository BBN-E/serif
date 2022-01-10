// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_TRIGGER_OBSERVATION_H
#define EVENT_TRIGGER_OBSERVATION_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/wordClustering/WordClusterClass.h"
#include "Generic/discTagger/DTObservation.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/SymbolUtilities.h"

#define ETO_MAX_WN_OFFSETS 20
#define ETO_MAX_OTHER_ARGS 3

class EventTriggerObservation : public DTObservation {
public:
	EventTriggerObservation() : DTObservation(_className),
		_wordCluster(WordClusterClass::nullCluster()), 
		_wordClusterMC(WordClusterClass::nullCluster()) {}

	~EventTriggerObservation() {}

	virtual DTObservation *makeCopy();

	void populate(EventTriggerObservation *other);	
	void populate(int token_index, const TokenSequence *tokens, const Parse *parse,
		MentionSet *mentionSet, const PropositionSet *propSet, bool use_wordnet);

	Symbol getPOS();
	Symbol getStemmedWord();
	Symbol getWord();
	Symbol getLCWord();
	bool isLastWord();
	bool isFirstWord();
	Symbol getNextWord();
	Symbol getSecondNextWord();
	Symbol getPrevWord();
	Symbol getSecondPrevWord();
	int getNthOffset(int n);
	int getReversedNthOffset(int n);
	int getNOffsets();
	WordClusterClass getWordCluster();
	WordClusterClass getWordClusterMC();
	Symbol getObjectOfTrigger();
	Symbol getIndirectObjectOfTrigger();
	Symbol getSubjectOfTrigger();
	Symbol getOtherArgToTrigger(int n);
	Symbol getDocumentTopic();
	void setDocumentTopic(Symbol topic);
	bool isNominalPremod();
	bool isCopula();
	Symbol getTriggerSyntacticType(){
		return _triggerSyntacticType;
	};


	static const int _MAX_OTHER_ARGS;

private:
	static const Symbol _className;

	Symbol _word;
	Symbol _nextWord;
	Symbol _secondNextWord;
	Symbol _prevWord;
	Symbol _secondPrevWord;
	Symbol _lcWord;
	Symbol _stemmedWord;
	Symbol _pos;
	Symbol _subjectOfTrigger;
	Symbol _objectOfTrigger;
	Symbol _indirectObjectOfTrigger;
	Symbol _otherArgsToTrigger[ETO_MAX_OTHER_ARGS];
	WordClusterClass _wordCluster;
	WordClusterClass _wordClusterMC;
	int _wordnetOffsets[ETO_MAX_WN_OFFSETS];
	int _n_offsets;
	Symbol _documentTopic;
	bool _is_nominal_premod;
	bool _is_copula;
	Symbol _triggerSyntacticType;
};

#endif
