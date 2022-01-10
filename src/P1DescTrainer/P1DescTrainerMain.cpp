// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/descriptors/discmodel/P1DescTrainer.h"
#include "Generic/common/FeatureModule.h"

using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "P1DescTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);
		FeatureModule::load();
		std::string log_file = ParamReader::getRequiredParam("trainer_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		const wchar_t *context_name = L"desc-training";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);

		P1DescTrainer trainer;

		char devtest[500];
		if (ParamReader::getParam("p1_desc_devtest",devtest, 500))		{
			if (strcmp(devtest, "true") == 0) {
				trainer.devTest();
				exit(0);
			}
		}

		trainer.train();
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

	return 0;
}
