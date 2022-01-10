// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// PIdFSimulatedActiveLearning.cpp : Defines the entry point for the console application.
//

#include "common/leak_detection.h"

#include <stdio.h>
#include <crtdbg.h>

#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "names/discmodel/PIdFSimActiveLearningTrainer.h"


using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "PIdfSimActiveLearningTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		PIdFSimActiveLearningTrainer trainer;
		trainer.addTrainingSentencesFromTrainingFile();
		trainer.addActiveLearningSentencesFromTrainingFileList();
		trainer.doActiveLearning();
	
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

#ifdef _DEBUG
		cerr << "Press enter to exit....\n";
		getchar();
#endif

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

#if 0
	ParamReader::finalize();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

#ifdef _DEBUG
	cerr << "Press enter to exit....\n";
	getchar();
#endif

	return 0;
}
