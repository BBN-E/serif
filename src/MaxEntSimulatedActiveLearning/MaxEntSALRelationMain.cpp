// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// based on PIdFSimulatedActiveLearning.cpp

#include "Generic/common/leak_detection.h"

#include <stdio.h>
#if defined(WIN32) || defined(WIN64)
#include <crtdbg.h>
#endif
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "InstanceSetTester.h"
#include "MaxEntSALRelationTrainer.h"  
#include "Utilities.h"
#include "Generic/common/FeatureModule.h"

using namespace std;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "MaxEntSimulatedActiveLearningMain.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		// Load modules specified in the parameter file.
		FeatureModule::load();

/*		InstanceSetTester tester;
		tester.runTest();
		exit(0);
*/

		char log_file[500];
		ParamReader::getRequiredNarrowParam(log_file, Symbol(L"maxent_sal_log_file"), 500);

		const wchar_t *context_name = L"maxent-sal";
		SessionLogger::logger = _new FileSessionLogger(log_file, 1, &context_name);

		MaxEntSALRelationTrainer trainer;
		int modelNum = ParamReader::getRequiredIntParam(Symbol(L"maxent_sal_starting_modelnum"));

		if (ParamReader::getRequiredTrueFalseParam(Symbol(L"maxent_sal_devtest"))) {

			printf("MaxEntSALRelationMain::main():  Running devtest on model %d...\n", modelNum);
			trainer.devTest(modelNum);

		} else {  // no devtest, regular training

			bool human = ParamReader::getRequiredTrueFalseParam(Symbol(L"maxent_sal_human"));
			trainer.doActiveLearning(human, modelNum);
		}
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
