#ifndef EN_EVENT_VALUE_RECOGNIZER_H
#define EN_EVENT_VALUE_RECOGNIZER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/values/DeprecatedEventValueRecognizer.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"

class EventMention;
class ValueMention;
class Mention;
class SynNode;
class Proposition;
class SentenceTheory;
class EventPatternMatcher;

class EnglishDeprecatedEventValueRecognizer: public DeprecatedEventValueRecognizer {
private:
	friend class EnglishDeprecatedEventValueRecognizerFactory;

	void addPosition(DocTheory* docTheory, EventMention *vment);
	bool isJobTitle(Symbol word);
	int _n_value_mentions;
	Symbol getBaseType(Symbol fullType);

	void addValueMention(DocTheory *docTheory, EventMention *vment, 
						const Mention *ment, Symbol valueType, Symbol argType, float score);
	void addValueMention(DocTheory *docTheory, EventMention *vment, 
						const SynNode *node, Symbol valueType, Symbol argType, float score);
	const Mention *getNthMentionFromProp(SentenceTheory *sTheory, const Proposition *prop, int n);
	const SynNode *getNthPredHeadFromProp(const Proposition *prop, int n);

	ValueMention *_valueMentions[MAX_DOCUMENT_VALUES];

	EventPatternMatcher *_matcher;
	Symbol TOPLEVEL_PATTERNS;
	Symbol CERTAIN_SENTENCES;

	void transferCrime(EventMention *vment, class MentionSet *mentionSet, 
			const class PropositionSet *propSet, class EventMentionSet* vmSet);

	EnglishDeprecatedEventValueRecognizer();

public:
	void createEventValues(DocTheory* docTheory);
	
};


class EnglishDeprecatedEventValueRecognizerFactory: public DeprecatedEventValueRecognizer::Factory {
	virtual DeprecatedEventValueRecognizer *build() { return _new EnglishDeprecatedEventValueRecognizer(); } 
};
 
#endif
