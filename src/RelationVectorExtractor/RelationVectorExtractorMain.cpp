// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "common/UnrecoverableException.h"
#include "driver/SessionProgram.h"
#include "CASerif_generic/results/CAResultCollector.h"
#include "CASerif_generic/driver/CADocumentDriver.h"
#include "RelationVectorExtractor/CAVectorResultCollector.h"
#include "common/HeapStatus.h"


using namespace std;

/**
 * Prints utility usage information to standard error.
 */
void print_usage()
{
	cerr << "usage: RelationVectorExtractor config" << endl;
	cerr << "    config				name of Serif configuration file" << endl << endl;
}

/**
 * Program entry point.
 *
 * @param argc the number of command-line arguments.
 * @param argv the command-line arguments.
 */
int main(int argc, char* argv[])
{

/*#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

	_crtBreakAlloc = 9999223;
#endif*/

	if (argc != 2) {
		print_usage();
		return -1;
	}


	try {
		ParamReader::readParamFile(argv[1]);

		SessionProgram sessionProgram;
		sessionProgram.setStageActiveness("START", true);
		sessionProgram.setStageActiveness("tokens", true);
		sessionProgram.setStageActiveness("parse", true);
		sessionProgram.setStageActiveness("mentions", true);
		sessionProgram.setStageActiveness("props", true);
		sessionProgram.setStageActiveness("metonymy", true);
		sessionProgram.setStageActiveness("entities", true);
		sessionProgram.setStageActiveness("events", false);
		sessionProgram.setStageActiveness("relations", true);
		sessionProgram.setStageActiveness("doc-entities", false);
		sessionProgram.setStageActiveness("doc-relations", false);
		sessionProgram.setStageActiveness("generics", false);
		sessionProgram.setStageActiveness("output", true);
		sessionProgram.setStageActiveness("score", false);
		sessionProgram.setStageActiveness("END", false);
		
		CAResultCollector *resultCollector = new CAVectorResultCollector();
		CADocumentDriver documentDriver(&sessionProgram,
			  						    resultCollector);

		documentDriver.run();
		((CAVectorResultCollector *)resultCollector)->printVectors();

		delete resultCollector;
	}
	catch (UnrecoverableException &e) {
		cerr << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

/*#ifdef ENABLE_LEAK_DETECTION
	ParamReader::finalize();
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif*/

	return 0;
}
