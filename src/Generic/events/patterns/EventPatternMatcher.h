// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_PATTERN_MATCHER_H
#define EVENT_PATTERN_MATCHER_H

#include "Generic/common/Symbol.h"
#include "Generic/common/limits.h"
#include <map>

class Sexp;
class EventPatternSet;
class EventPatternNode;
class EventSearchNode;
class PropositionSet;
class Proposition;
class Mention;
class ValueMention;
class MentionSet;
class ValueMentionSet;
class EntitySet;

class EventPatternMatcher {
public:
	EventPatternMatcher(const EventPatternSet* set);
	~EventPatternMatcher();

	void resetForNewSentence(const PropositionSet *props,
		MentionSet *mentions, ValueMentionSet *values);

	EventSearchNode *matchNode(const Mention *ment, Symbol sym);
	EventSearchNode *matchAllPossibleNodes(const Proposition *prop, Symbol sym);
	EventSearchNode *matchNode(const Mention *ment, const EventPatternNode *node);
	EventSearchNode *matchNode(const Proposition *prop, Symbol sym);
	EventSearchNode *matchNode(const Proposition *prop, const EventPatternNode *node);

private:
	const EventPatternSet* _patternSet;
	MentionSet* _mentionSet;
	ValueMentionSet *_valueSet;

	std::map<int, Proposition*> _definingProps;
	std::map<int, Proposition*> _definingSetProps;
	std::map<int, ValueMention*> _mentionsAsValues;
	EventSearchNode *createMatchedMentionResult(const Mention *ment, Symbol label);

	void collectDefinitions(const PropositionSet *propSet);
	void setMentionsAsValues();
	Symbol getEntityOrValueType(const Mention *mention);
	bool ALLOW_TYPE_OVERRIDING;
	int _stack_depth;

};

#endif
