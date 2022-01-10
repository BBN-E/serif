// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/relations/MaxEntRelationTrainer.h"

#include <time.h>

using namespace std;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "MaxEntRelationTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		std::string log_file = ParamReader::getRequiredParam("maxent_trainer_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		const wchar_t *context_name = L"relation-training";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);

		time_t ltime;
		time(&ltime);
		wchar_t time_str[100];
		wcsftime(time_str, 100, L"%Y-%m-%d %H:%M:%S", localtime(&ltime));
		*(SessionLogger::logger) << "Start Time: " << time_str << "\n";
		cout << "Start Time: " << time_str << "\n";

		MaxEntRelationTrainer trainer;

		char devtest[500];
		if (ParamReader::getParam("maxent_relation_devtest",devtest, 500))		{
			if (strcmp(devtest, "true") == 0) {
				trainer.devTest();
				exit(0);
			}
		}

		trainer.train();

		time(&ltime);
		wcsftime(time_str, 100, L"%Y-%m-%d %H:%M:%S", localtime(&ltime));
		*(SessionLogger::logger) << "End Time: " << time_str << "\n";
		cout << "End Time: " << time_str << "\n";

	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
