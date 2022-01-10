// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DOC_RELATION_EVENT_PROCESSOR_H
#define DOC_RELATION_EVENT_PROCESSOR_H

class DocTheory;
#include "Generic/docRelationsEvents/DocEventHandler.h"
#include "Generic/docRelationsEvents/DocumentRelationFinder.h"
#include "Generic/docRelationsEvents/RelationPromoter.h"

class CorrectAnswers;

#include "dynamic_includes/common/ProfilingDefinition.h"
#include "Generic/common/GenericTimer.h"

class DocRelationEventProcessor {
public:
	DocRelationEventProcessor();
	~DocRelationEventProcessor();
	DocRelationEventProcessor(bool use_correct_relations, bool use_correct_events);
	void cleanup();

	void setCorrectAnswers(CorrectAnswers *correctAnswers) 
	{
		_docEventHandler->_correctAnswers = correctAnswers;
		_docRelationFinder->_correctAnswers = correctAnswers;
	}

	void doDocRelationsAndDocEvents(DocTheory *docTheory);

	const DTTagSet * getEventTypeTagSet();
	const DTTagSet * getEventArgumentTypeTagSet();

private:
	RelationPromoter *_relationPromoter;
	DocumentRelationFinder *_docRelationFinder;
	DocEventHandler *_docEventHandler;

public:
	mutable GenericTimer relationPromoterLoadTimer;
	mutable GenericTimer relationLoadTimer;
	mutable GenericTimer eventLoadTimer;

	mutable GenericTimer relationPromoterProcessTimer;
	mutable GenericTimer relationProcessTimer;
	mutable GenericTimer docRelationProcessTimer;
	mutable GenericTimer eventProcessTimer;

	void logTrace();
};

#endif
