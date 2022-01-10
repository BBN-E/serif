// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/discourseRel/DiscourseRelTrainer.h"
//#include "discourseRel/PennDiscourseTreebank.h"
using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "DiscourseRelTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	
	// code development stage - test PennDiscourseTreebank.cpp
	/* PennDiscourseTreebank::loadDataFromPDTBFileList (argv[1]);
	
	try{
		//Symbol label = PennDiscourseTreebank::getLabelofExpConnective ("0004", "because", "7", "18");
		Symbol label = PennDiscourseTreebank::getLabelofExpConnective ("0003", "because", "7", "18");
		std::cout << "is a connective \n";
	}
	catch (UnexpectedInputException &e) {
		std::cout << "is not a connective \n";
	}
	

	Symbol label = PennDiscourseTreebank::getLabelofExpConnective ("0003", "because", "7", "18");
	
	if (lable == 0){
		std::cout << "is not a connective \n";	
	}else{
		std::cout << "is a connective \n";	
	}

	*/
	

	try {
		ParamReader::readParamFile(argv[1]);

		char log_file[500];
		if (!ParamReader::getParam("trainer_log_file",log_file, 500))		{
			throw UnexpectedInputException("main()",
				"Parameter 'trainer_log_file' not specified");
		}

		const wchar_t *context_name = L"discourse-relation-training";
		SessionLogger::logger = _new FileSessionLogger(log_file,
									   1, &context_name);

		DiscourseRelTrainer trainer;
		char devtest[500];
		if (ParamReader::getParam("discourse_rel_devtest",devtest, 500))		{
			if (strcmp(devtest, "true") == 0) {
				//trainer.devTest();
				exit(0);
			}
		}

		trainer.train();

		ParamReader::finalize();
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
