// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "KbaStreamCorpus/KbaStreamCorpusModule.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/common/ParamReader.h"
#include "Generic/commandLineInterface/CommandLineInterface.h"
#include "Generic/driver/QueueDriver.h"
#include "Generic/driver/DiskQueueDriver.h"
#include "KbaStreamCorpus/KbaStreamCorpusQueueFeeder.h"
#include "KbaStreamCorpus/KbaStreamCorpusDocumentDriver.h"
#include "Generic/commandLineInterface/CommandLineInterface.h"
#include "Generic/results/SerifXMLResultCollector.h"
#include "Generic/driver/SessionProgram.h"



struct KbaStreamCorpusCLIHook: public ModifyCommandLineHook {
	WhatNext run(int verbosity) { 
		if (ParamReader::isParamTrue("use_stream_corpus_driver")) {
			KbaStreamCorpusDocumentDriver docDriver;
			SessionProgram sessionProgram;
			SerifXMLResultCollector resultCollector;

			GenericTimer totalLoadTimer;
			totalLoadTimer.startTimer();
			docDriver.beginBatch(&sessionProgram, &resultCollector);
			totalLoadTimer.stopTimer();

			GenericTimer totalProcessTimer;
			totalProcessTimer.startTimer();
			docDriver.runOnStreamCorpusChunks();
			docDriver.endBatch();
			totalProcessTimer.stopTimer();

			reportProfilingInformation(totalLoadTimer, totalProcessTimer, docDriver);

			SessionLogger::info("kba-stream-corpus")
				<< "Load time: " << totalLoadTimer.getTime() << " msec\n"
				<< "Process time: " << totalProcessTimer.getTime() << " msec";

			return SUCCESS;
		} else {
			return CONTINUE;
		}
	}
};

// Plugin setup function
extern "C" DLL_PUBLIC void* setup_KbaStreamCorpus() {
	addModifyCommandLineHook(boost::shared_ptr<KbaStreamCorpusCLIHook>(
			_new KbaStreamCorpusCLIHook()));

	// Register queue feeder
	QueueDriver::addImplementation<KbaStreamCorpusQueueFeeder<DiskQueueDriver> >("kba-stream-corpus-disk-feeder");

	return FeatureModule::setup_return_value();
}
