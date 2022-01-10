// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/relations/discmodel/P1RelationTrainer.h"

using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "P1RelationTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		std::string log_file = ParamReader::getRequiredParam("p1_trainer_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		const wchar_t *context_name = L"relation-training";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);

		P1RelationTrainer trainer;
			
		char devtest[500];
		if (ParamReader::getNarrowParam(devtest, Symbol(L"p1_relation_devtest"), 500))
		{
			if (strcmp(devtest, "true") == 0) {
				trainer.devTest();
				exit(0);
			}
		} 

		trainer.train();

	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		cerr << "Error Source: " << e.getSource() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");
		return -1;
	}
	catch (exception &e) {
		cerr << "\nError: " << e.what() << " in P1RelationTrainerMain::main()\n";
		HeapChecker::checkHeap("main(); About to exit due to error");
		return -1;
	}


	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
