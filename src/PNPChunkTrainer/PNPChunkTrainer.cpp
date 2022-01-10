// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// PNPChunkTrainer.cpp : Defines the entry point for the console application.
//


#include "common/leak_detection.h"

#include <stdio.h>
#if defined(WIN32) || defined(WIN64)
#include <crtdbg.h>
#endif

#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "PNPChunking/PNPChunkTrainer.h"
#include "PNPChunking/PNPChunkDecoder.h"

using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "PNPChunkTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		char buffer[500];
		ParamReader::getRequiredParam("pnpchunk_trainer_standalone_mode", buffer, 500);

		if (strcmp(buffer, "train") == 0) {
			PNPChunkTrainer trainer;
			trainer.train();
		}
		else if (strcmp(buffer, "decode") == 0) {
			PNPChunkDecoder decoder;
			decoder.decode();
		}
		else if (strcmp(buffer, "devtest") == 0) {
			PNPChunkDecoder decoder;
			decoder.devTest();
		}
		else {
			throw UnexpectedInputException("PNPChunkTrainer::main()",
				"Parameter 'pnpchunk_trainer_standalone_mode' invalid");
		}
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
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
