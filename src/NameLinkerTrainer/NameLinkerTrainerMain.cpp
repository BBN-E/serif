// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/edt/NameLinkerTrainer/NameLinkerTrainer.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <iostream>

#include "common/version.h"

int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cerr << "\nTrainer usage: param_file\n";
		return -1;
	}
	try {
		ParamReader::readParamFile(argv[1]);

		std::string model_prefix = ParamReader::getRequiredParam("name_linker_model");
		std::string log_file = model_prefix + ".log";
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		
		const wchar_t *context_name = L"name-linker-trainer";
		SessionLogger::setGlobalLogger(_new FileSessionLogger(log_file_as_wstring.c_str(), 1, &context_name));
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "This is NameLinkerTrainer for the ACE task, version 1.00.\n"
							   << "Serif version: " << SerifVersion::getVersionString() << "\n"
							   << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
							   << "\n";
		*SessionLogger::logger << "_________________________________________________\n"
							   << "Starting NameLinkerTrainer Session\n"
							   << "Parameters:\n";

		ParamReader::logParams();
		*SessionLogger::logger << "\n";

		NameLinkerTrainer trainer(NameLinkerTrainer::TRAIN);
		trainer.train();

		ParamReader::finalize();

		cerr << endl;
		SessionLogger::logger->displaySummary();
		cerr << endl;

		delete SessionLogger::logger;

	} catch (UnrecoverableException &e) {

		e.putMessage(std::cerr);
		return -1;
	}


	return 0;
}
