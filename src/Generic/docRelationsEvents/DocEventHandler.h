// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_EVENT_HANDLER_H
#define DOC_EVENT_HANDLER_H

#include "Generic/events/EventLinker.h"
#include "Generic/events/EventFinder.h"
#include "Generic/common/SymbolHash.h"
#include <vector>

class Event;
class DocTheory;
class EventMention;
class StatEventLinker;

#include "Generic/CASerif/correctanswers/CorrectAnswers.h"

class DocEventHandler {
public:
	DocEventHandler(bool use_correct_events = false);
	~DocEventHandler();
	
	CorrectAnswers *_correctAnswers;
	bool _use_correct_answers;
	bool _allow_events_to_be_mentions;
	bool _allowMultipleEventsOfDifferentTypeWithSameAnchor;
	bool _allowMultipleEventsOfSameTypeWithSameAnchor;

	void createEventSet(DocTheory* docTheory);

	static Symbol assignTopic(const DocTheory* docTheory);
	static Symbol assignTopic(TokenSequence **tokenSequences, Parse **parses, 
							   int start_index, int max); 
	
	const DTTagSet * getEventTypeTagSet();
	const DTTagSet * getArgumentTypeTagSet();

private:

	EventLinker *_eventLinker;
	StatEventLinker *_statEventLinker;
	EventFinder *_eventMentionFinder;
	int _event_linking_style;
	enum {SENTENCE, DOCUMENT, NONE};
	const bool _use_correct_events;

   	std::vector<EventMention*> _allVMentions;
	std::vector<Event*> _events;
   	std::vector< std::vector<float> > _linkScores;
	float _link_threshold;
	void findEventMentions(DocTheory* docTheory);
	void doSentenceLevelEventLinking(DocTheory* docTheory);
	void doDocumentLevelEventLinking(DocTheory* docTheory, int n_vmentions);
	void assignEventAttributes(EventSet *eventSet);
	int filterEventMentionSet(DocTheory* docTheory, MentionSet *mentionSet, 
		EventMentionSet *eventMentionSet);
	void killEventMention(EventMention *vm, const char *reason);
	void mergeEventMentions(EventMention *keepMent, EventMention *elimMent);

	void transferArgsBetweenConnectedEvents(DocTheory* docTheory,
		EventMention *vm1, EventMention *vm2);
	void transferArgs(EventMention *fromVM, EventMention *toVM, Symbol role);
	Symbol getBaseType(Symbol fullType);

	Symbol ignoreSym;
	bool _use_preexisting_event_values;

	static Symbol *_topics;
	static SymbolHash **_topicWordSets;
	static int *_topicWordCounts;
	static int _n_topics;
	static bool _topics_use_only_nouns;
	static bool _topics_initialized;
	static void initializeDocumentTopics();
	

	Symbol _documentTopic;

	UTF8OutputStream _debugStream;
	bool DEBUG;

};

#endif
