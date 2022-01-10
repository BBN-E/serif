// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// MorphAnalyzer.cpp : Defines the entry point for the console application.
//

#include "Generic/common/leak_detection.h"

#include <stdio.h>
#include "time.h"
#if defined(WIN32) || defined(WIN64)
#include <crtdbg.h>
#endif


#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "MorphAnalyzer/Processor.h"


int main(int argc, char **argv) {
	if (argc != 3) {
		cerr << "\nUSAGE: MorphAnalyzer.exe <param_file> <corpus_file>\n";
		return -1;
	}

	try {
		char * paramfile = argv[1];
		char * corpus_file = argv[2];
		char output_file[500];
		strcpy(output_file, corpus_file);
		strncat(output_file, ".tokens", 450);

		Processor * processor = _new Processor(paramfile);
		cout << "Creating analyzed corpus...\n";
		processor->createAnalyzedCorpus(corpus_file, output_file);
		cout << "Tokens file has been saved to " << output_file << "\n";
		delete processor;
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
