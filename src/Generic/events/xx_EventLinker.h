// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/events/EventLinker.h"


class GenericEventLinker : public EventLinker {
private:
	friend class GenericEventLinkerFactory;

public:
	void linkEvents(const EventMentionSet *eventMentionSet, 
		EventSet *eventSet,
		const EntitySet *entitySet,
		const PropositionSet *propSet,
		CorrectAnswers *correctAnswers,
		int sentence_num,
		Symbol docname) {}

private:
	GenericEventLinker() {}
};

class GenericEventLinkerFactory: public EventLinker::Factory {
	virtual EventLinker *build() { return _new GenericEventLinker(); }
};

