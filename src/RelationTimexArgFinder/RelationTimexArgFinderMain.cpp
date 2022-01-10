// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/docRelationsEvents/RelationTimexArgFinder.h"

using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "RelationTimexArgFinder.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		std::string log_file = ParamReader::getRequiredParam("relation_time_finder_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());

		const wchar_t *context_name = L"relation-time-training";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);


		char temp[501];
		bool train = false;
		bool round_robin = false;
		bool devtest = false;
		if (ParamReader::getParam("relation_time_training_file_list", temp, 500))			
			train = true;
		if (ParamReader::getParam("relation_time_round_robin_setup", temp, 500))			
			round_robin = true;
		if (ParamReader::getParam("relation_time_devtest_file_list", temp, 500))		
			devtest = true;
	
		if (train) {
			RelationTimexArgFinder *trainer = _new RelationTimexArgFinder(RelationTimexArgFinder::TRAIN);
			trainer->train();
		}
		else if (round_robin) {
			RelationTimexArgFinder *trainer = _new RelationTimexArgFinder(RelationTimexArgFinder::DECODE);
			trainer->roundRobin();
		}
		else if (devtest) {
			RelationTimexArgFinder *trainer = _new RelationTimexArgFinder(RelationTimexArgFinder::DECODE);
			trainer->devtest();
		}
		else {
			throw UnexpectedInputException("main()",
				"One of 'relation_time_training_file_list', 'relation_time_round_robin_setup' or 'relation_time_devtest_file_list' must be specified.");
		}
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
