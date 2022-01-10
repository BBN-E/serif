// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/UnrecoverableException.h"
#include "edt/NameLinkerTrainer/NameLinkerTrainer.h"
#include "common/SessionLogger.h"
#include "common/ParamReader.h"
#include "ASR/sentBreaker/ASRSentBreakerTrainer.h"
#include "ASR/sentBreaker/ASRSentBreaker.h"

#include <iostream>


using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "ASRSentBreaker.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		Symbol taskParam = ParamReader::getParam(L"task");
		if (taskParam == Symbol(L"train")) {
			ASRSentBreakerTrainer trainer;
			trainer.train();
		}
		else if (taskParam == Symbol(L"decode")) {
			ASRSentBreaker decoder;
			decoder.decode();
		}
		else {
			cerr << "ASRSentBreaker requires a 'task' parameter. This should be 'train' or\n"
				    "'decode'\n";
			return -1;
		}

		ParamReader::finalize();

	}
	catch (UnrecoverableException &e) {
		e.putMessage(cerr);
		cerr << "\n";
		return -1;
	} 
	

	return 0;
}
