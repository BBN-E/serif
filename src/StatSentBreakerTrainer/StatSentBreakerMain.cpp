// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/sentences/StatSentBreakerTrainer.h"

#include <iostream>


using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "StatSentBreakerTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		std::string log_file = ParamReader::getRequiredParam("log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());

		const wchar_t *context_name = L"sent-breaker-training";
		SessionLogger::logger = new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);

		Symbol taskParam = ParamReader::getParam(L"task");
		if (taskParam == Symbol(L"train")) {
			StatSentBreakerTrainer trainer(StatSentBreakerTrainer::TRAIN);
			trainer.train();
		}
		else if (taskParam == Symbol(L"devtest")) {
			StatSentBreakerTrainer trainer(StatSentBreakerTrainer::DEVTEST);
			trainer.devtest();
		}
		else {
			cerr << "StatSentBreakerTrainer requires a 'task' parameter. This should be 'train' or\n"
				    "'devtest'\n";
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
