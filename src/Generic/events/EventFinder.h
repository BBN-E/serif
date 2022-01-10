// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EVENT_FINDER_H
#define EVENT_FINDER_H

#include "Generic/common/Attribute.h"
#include "Generic/common/hash_map.h"
#include "Generic/common/limits.h"
#include "Generic/common/LocatedString.h"
#include "Generic/common/Offset.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8OutputStream.h"
#include <map>
#include <vector>

class DocTheory;
class DeprecatedPatternEventFinder;
class PatternEventFinder;
class StatEventFinder;
class ValueMention;
class ValueMentionSet;
class Token;
class TokenSequence;
class Parse;
class Mention;
class MentionSet;
class EntitySet;
class PropositionSet;
class SynNode;
class EventMentionSet;
class DTTagSet;



class EventFinder {
public:
	EventFinder(Symbol scope);
	~EventFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);

	EventMentionSet *getEventTheory(const TokenSequence *tokens,
		ValueMentionSet *valueMentionSet,
		Parse *parse,
		MentionSet *mentionSet,
		PropositionSet *propSet);

	void setDocumentTopic(Symbol topic);

	const DTTagSet * getEventTypeTagSet();
	const DTTagSet * getArgumentTypeTagSet();

	void allowMentionSetChanges();
	void disallowMentionSetChanges();

	bool isAlreadyAttachedToMention(ValueMention *val, EventMentionSet *eventMentionSet);
	bool attachToPreexistingMention(ValueMention *val, EventMentionSet *eventMentionSet, int valtype);
		
	static bool DEBUG;
	static UTF8OutputStream _debugStream;

	// Note: this is public to allow access by CorrectAnswerSerif
	DeprecatedPatternEventFinder *_deprecatedPatternEventFinder;

	EventMentionSet* filterEventTheory(DocTheory* docTheory, EventMentionSet* eventMentionSet);
private:
	struct ExternalEvent {
		Symbol event_type;
		int primary_st;
		int primary_end;
		int secondary_st;
		int secondary_end;
		int props_st;
		int props_end;
	};
	typedef ExternalEvent* ExternalEvent_ptr;
	struct ExternalEvents {
		std::vector<ExternalEvent_ptr> vect;
	};
	typedef ExternalEvents* ExternalEvents_ptr;
	struct ExtEventHashOp {
		size_t operator()(const Symbol& s) const {
			return s.hash_code();
		}
	};
	struct ExtEventEqualOp {
		bool operator()(const Symbol& s1, const Symbol& s2) const {
			return (s1==s2);
		}
	};

	typedef std::map<Symbol, ExternalEvents*> ExternalEventsMap;
	void printEventMentionSet(EventMentionSet *vmSet, const TokenSequence *tokens, Parse *parse);

private:
	
	PatternEventFinder *_patternEventFinder;
	StatEventFinder *_statEventFinder;

	bool _allow_mention_set_changes;
	bool _use_preexisting_event_values;
	bool _add_external_events;

	ExternalEventsMap _external_events;
	int _n_event_value_types;
	std::vector<std::string> _event_filters;
	DocTheory * _doc_theory;
    Symbol *_evType;
	Symbol *_evValidRole;
	Symbol **_evValidEventTypes;
	void addFakedMentions(EventMentionSet *targetSet, const TokenSequence *tokens, ValueMentionSet *valueMentionSet, 
		Parse *parse, PropositionSet *propSet);
	void loadExternalEvents(std::string filepath);
	EventMentionSet* addExternalEvents(EventMentionSet *targetSet, const TokenSequence *tokens, Parse *parse, MentionSet *mentionSet);

};


#endif
