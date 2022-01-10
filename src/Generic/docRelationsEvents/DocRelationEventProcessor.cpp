// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/SessionLogger.h"
#include "Generic/docRelationsEvents/DocRelationEventProcessor.h"
#include "Generic/docRelationsEvents/RelationPromoter.h"
#include "Generic/docRelationsEvents/DocumentRelationFinder.h"
#include "Generic/docRelationsEvents/DocEventHandler.h"

#include <iostream>
using namespace std;


DocRelationEventProcessor::DocRelationEventProcessor() {

	relationPromoterLoadTimer.startTimer();
	_relationPromoter = _new RelationPromoter();
	relationPromoterLoadTimer.stopTimer();

	relationLoadTimer.startTimer();
	_docRelationFinder = _new DocumentRelationFinder();
	relationLoadTimer.stopTimer();

	eventLoadTimer.startTimer();
	_docEventHandler = _new DocEventHandler();
	eventLoadTimer.stopTimer();
}

DocRelationEventProcessor::~DocRelationEventProcessor() {

	delete _relationPromoter;
	delete _docRelationFinder;
	delete _docEventHandler;

}

DocRelationEventProcessor::DocRelationEventProcessor(bool use_correct_relations, bool use_correct_events) {
	relationPromoterLoadTimer.startTimer();
	_relationPromoter = _new RelationPromoter();
	relationPromoterLoadTimer.stopTimer();

	relationLoadTimer.startTimer();
	_docRelationFinder = _new DocumentRelationFinder(use_correct_relations);
	relationLoadTimer.stopTimer();

	eventLoadTimer.startTimer();
	_docEventHandler = _new DocEventHandler(use_correct_events);
	eventLoadTimer.stopTimer();
}

void DocRelationEventProcessor::cleanup() {
	_docRelationFinder->cleanup();
}

void DocRelationEventProcessor::doDocRelationsAndDocEvents(DocTheory *docTheory) {
	relationProcessTimer.startTimer();
	_docRelationFinder->findSentenceLevelRelations(docTheory);
	relationProcessTimer.stopTimer();

	eventProcessTimer.startTimer();
	_docEventHandler->createEventSet(docTheory);
	eventProcessTimer.stopTimer();

	docRelationProcessTimer.startTimer();
	// so that we can use the event set
	_docRelationFinder->findDocLevelRelations(docTheory);
	docRelationProcessTimer.stopTimer();

	relationPromoterProcessTimer.startTimer();
	_relationPromoter->promoteRelations(docTheory);
	relationPromoterProcessTimer.stopTimer();
}

void DocRelationEventProcessor::logTrace() {
	SessionLogger::info("profiling") << "DocRelationEvent Load Time:" << endl;
	SessionLogger::info("profiling") << "relation\t" << relationLoadTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << "event\t" << eventLoadTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << "relPromoter\t" << relationPromoterLoadTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << endl;

	SessionLogger::info("profiling") << "DocRelationEvent Process Time:" << endl;
	SessionLogger::info("profiling") << "relation\t" << relationProcessTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << "docRelation\t" << docRelationProcessTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << "event\t" << eventProcessTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << "relPromoter\t" << relationPromoterProcessTimer.getTime() << " msec" << endl;
	SessionLogger::info("profiling") << endl;
}

const DTTagSet* DocRelationEventProcessor::getEventTypeTagSet() {
	return _docEventHandler->getEventTypeTagSet();
}

const DTTagSet* DocRelationEventProcessor::getEventArgumentTypeTagSet() {
	return _docEventHandler->getArgumentTypeTagSet();
}
