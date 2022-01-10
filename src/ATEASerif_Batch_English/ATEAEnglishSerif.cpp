// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "common/version.h"
#include "English/common/en_version.h"

#include "driver/DocumentDriver.h"
#include "driver/SessionProgram.h"
#include "common/ParamReader.h"
#include "apf/APFResultCollector.h"
#include "eeml/EEMLResultCollector.h"
#include "common/HeapChecker.h"
#include "common/UnrecoverableException.h"
#include "state/ObjectIDTable.h"
#include "ATEASerif_generic/results/ATEAResultCollector.h"
#include <iostream>
#include <stdio.h>


using namespace std;

#undef ENABLE_LEAK_DETECTION


int main(int argc, char **argv) {

#ifdef _DEBUG
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

//	_crtBreakAlloc = 9999223;
#endif

	cout << "This is English Serif for the ACE task, version 1.00.\n"
		 << "Serif version: " << SerifVersion::getVersionString() << "\n"
		 << "\n";

	if (argc != 2) {
		cerr << "EnglishSerif.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		SessionProgram sessionProgram;
		ATEAResultCollector* resultCollector;

/*		char outputFormat[500];
		if (!ParamReader::getParam("output_format",outputFormat, 500))			resultCollector = new APFResultCollector;
		else if (!strcmp(outputFormat, "EEML"))
			resultCollector = new EEMLResultCollector();
		else if (!strcmp(outputFormat, "APF"))
			resultCollector = new APFResultCollector();
		else
			throw UnexpectedInputException("SerifEnglish::main()",
								   "Invalid output-format");
	*/
		resultCollector = new ATEAResultCollector(ATEAResultCollector::APF2005);

		DocumentDriver documentDriver(&sessionProgram,
									  resultCollector);

		// for memory checking, run twice
		int n_runs = 1;
		if (ParamReader::getParam(L"check_memory") == Symbol(L"true"))
			n_runs = 2;

		for (int i = 0; i < n_runs; i++) {
			documentDriver.run();
		}

		delete resultCollector;
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

#if defined(_DEBUG) || defined(_UNOPTIMIZED)
		printf("Press enter to exit....\n");
		getchar();
#endif

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

#ifdef ENABLE_LEAK_DETECTION
	ParamReader::finalize();
	ObjectIDTable::finalize();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

#ifdef _DEBUG
	printf("Press enter to exit....\n");
	getchar();
#endif

	return 0;
}
