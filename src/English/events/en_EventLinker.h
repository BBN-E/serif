// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/limits.h"
#include "Generic/events/EventLinker.h"
#include <vector>
class Event;
class EventMention;
class PropositionSet;

class EnglishEventLinker : public EventLinker {
private:
	friend class EnglishEventLinkerFactory;

public:
	void linkEvents(const EventMentionSet *eventMentionSet, 
							 EventSet *eventSet,
							 const EntitySet *entitySet,
							 const PropositionSet *propSet,
							 CorrectAnswers *correctAnswers,
							 int sentence_num,
							 Symbol docname);

private:
	EnglishEventLinker();
	Event *findBestCandidate(EventMention *em, const EntitySet *entitySet,
							 const PropositionSet *propSet);

    std::vector<Event*> _candidates;
	bool _use_correct_answers;

	bool anchorIsDefiniteNounPhrase(EventMention *em);

};

class EnglishEventLinkerFactory: public EventLinker::Factory {
	virtual EventLinker *build() { return _new EnglishEventLinker(); }
};

