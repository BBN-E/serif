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
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/ActorMentionFinder.h"
#include "Generic/theories/ActorMentionSet.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/EventMentionFinder.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/ICEWSDriver.h"
#include "Generic/icews/ICEWSDocumentReader.h"
#include "Generic/icews/ICEWSOutputWriter.h"
#include "Generic/icews/ActorMentionPattern.h"
#include "Generic/icews/EventMentionPattern.h"
#include "Generic/icews/ICEWSQueueFeeder.h"
#include "Generic/driver/DiskQueueDriver.h"

/** A command-line hook that checks if the icews_read_stories_from_database 
  * parameter is set, and if so it uses ICEWSDriver for the main loop, rather 
  * than the default DocumentDriver. */
struct ICEWSCommandLineHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) { 
		if (ParamReader::isParamTrue("icews_read_stories_from_database") && !ParamReader::hasParam("queue_driver")) {
			ICEWSDriver().processStories();
			return SUCCESS;
		} else {
			return CONTINUE; // use the normal document driver.
		}
	}
};

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_ICEWS() {
	// Add a new theory type & stage for actor mention detection
	DocTheory::registerSubtheoryType<ActorMentionSet>("ActorMentionSet");
	Stage icewsActorsStage = Stage::addNewStageBefore(Stage("output"), "icews-actors",
		"ICEWS actor mention detection");
	DocumentDriver::addDocTheoryStage<ActorMentionFinder>(icewsActorsStage);

	if (!ParamReader::isParamTrue("skip_icews_events")) {
		// Add a new theory type & stage for event mention detection
		DocTheory::registerSubtheoryType<ICEWSEventMentionSet>("ICEWSEventMentionSet");
		Stage icewsEventsStage = Stage::addNewStageBefore(Stage("output"), "icews-events",
			"ICEWS event mention detection");
		DocumentDriver::addDocTheoryStage<ICEWSEventMentionFinder>(icewsEventsStage);
	}

	// Add stage for writing events to the database.
	Stage icewsOutputStage = Stage::addNewStageBefore(Stage("output"), "icews-output",
		"Save ICEWS event mentions to the knowledge database");
	DocumentDriver::addDocTheoryStage<ICEWSOutputWriter>(icewsOutputStage);

	// Register ICEWS pattern types
	Pattern::registerPatternType<ICEWSActorMentionPattern>(Symbol(L"icews-actor"));
	Pattern::registerPatternType<ICEWSEventMentionPattern>(Symbol(L"icews-event"));

	// Register queue feeder
	QueueDriver::addImplementation<ICEWSQueueFeeder<DiskQueueDriver> >("icews-disk-feeder");

	// Register the command-line hook.
	addModifyCommandLineHook(boost::shared_ptr<ModifyCommandLineHook>(
			_new ICEWSCommandLineHook()));

	return FeatureModule::setup_return_value();
}
