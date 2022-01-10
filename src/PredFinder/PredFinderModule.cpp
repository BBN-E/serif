// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

/**
 * Defines a new Serif Stage that wraps the functionality of PredFinder.
 *
 * @file PredFinderModule.cpp
 * @author nward@bbn.com
 * @date 2012.01.31
 **/

#include "Generic/common/leak_detection.h"
#include "Generic/common/version.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/theories/DocTheory.h"
#include "PredFinderModule.h"
#include "PredFinder.h"

struct PredFinderHandler: public DocumentDriver::DocTheoryStageHandler {
	PredFinderHandler() : _predFinder(_new PredFinder()) { }
	~PredFinderHandler() { delete _predFinder; }
	void process(DocTheory* dt) { _predFinder->run(dt); }
private:
	PredFinder* _predFinder;
};

// Plugin setup function.
extern "C" DLL_PUBLIC void* setup_PredFinder() {
	// Initialize and register PredFinder stage, which does not save state
	Stage predFinderStage = Stage::addNewStageBefore(Stage("output"), "pred-finder", "Applies manual and LearnIt patterns to document to produce ELF relations", false);
	DocumentDriver::addDocTheoryStage<PredFinderHandler>(predFinderStage);

	return FeatureModule::setup_return_value();
}
