// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ICEWSModule.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/commandLineInterface/CommandLineInterface.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/driver/Stage.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/patterns/Pattern.h"
#include "ICEWS/EventMentionSet.h"
#include "ICEWS/EventMentionFinder.h"
#include "ICEWS/ActorMentionSet.h"
#include "ICEWS/ActorMentionFinder.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/ICEWSDriver.h"
#include "ICEWS/ICEWSDocumentReader.h"
#include "ICEWS/ICEWSOutputWriter.h"
#include "ICEWS/ActorMentionPattern.h"
#include "ICEWS/EventMentionPattern.h"
#include "ICEWS/ICEWSQueueFeeder.h"
#include "Generic/driver/DiskQueueDriver.h"

/** A command-line hook that checks if the icews_read_stories_from_database 
  * parameter is set, and if so it uses ICEWSDriver for the main loop, rather 
  * than the default DocumentDriver. */
struct ICEWSCommandLineHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) { 
		if (ParamReader::isParamTrue("icews_read_stories_from_database")) {
			ICEWS::ICEWSDriver().processStories();
			return SUCCESS;
		} else {
			return CONTINUE; // use the normal document driver.
		}
	}
};

struct ICEWSDocumentReaderFactory: public DocumentReader::Factory {
	DocumentReader *build() { return _new ICEWS::ICEWSDocumentReader(); }
	DocumentReader *build(Symbol defaultInputType) { return _new ICEWS::ICEWSDocumentReader(); }
};

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_ICEWS() {
	// Add a new theory type & stage for actor mention detection
	DocTheory::registerSubtheoryType<ICEWS::ActorMentionSet>("ICEWSActorMentionSet");
	Stage icewsActorsStage = Stage::addNewStageBefore(Stage("output"), "icews-actors",
		"ICEWS actor mention detection");
	DocumentDriver::addDocTheoryStage<ICEWS::ActorMentionFinder>(icewsActorsStage);

	// Add a new theory type & stage for event mention detection
	DocTheory::registerSubtheoryType<ICEWS::ICEWSEventMentionSet>("ICEWSEventMentionSet");
	Stage icewsEventsStage = Stage::addNewStageBefore(Stage("output"), "icews-events",
		"ICEWS event mention detection");
	DocumentDriver::addDocTheoryStage<ICEWS::ICEWSEventMentionFinder>(icewsEventsStage);

	// Add stage for writing events to the database.
	Stage icewsOutputStage = Stage::addNewStageBefore(Stage("output"), "icews-output",
		"Save ICEWS event mentions to the knowledge database");
	DocumentDriver::addDocTheoryStage<ICEWS::ICEWSOutputWriter>(icewsOutputStage);

	// Register the ICEWS_Sentence span type for serialization.	
	Span::registerSpanLoader(Symbol(L"ICEWS_Sentence"), ICEWS::loadIcewsSentenceSpan);

	// Register the ICEWS Document Reader
	DocumentReader::setFactory("icews_xmltext", boost::make_shared<ICEWSDocumentReaderFactory>());

	// Register ICEWS pattern types
	Pattern::registerPatternType<ICEWS::ICEWSActorMentionPattern>(Symbol(L"icews-actor"));
	Pattern::registerPatternType<ICEWS::ICEWSEventMentionPattern>(Symbol(L"icews-event"));

	// Register queue feeder
	QueueDriver::addImplementation<ICEWS::ICEWSQueueFeeder<DiskQueueDriver> >("icews-disk-feeder");

	// Register the command-line hook.
	addModifyCommandLineHook(boost::shared_ptr<ModifyCommandLineHook>(
			_new ICEWSCommandLineHook()));

	return FeatureModule::setup_return_value();
}
