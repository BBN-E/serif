// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/edt/PronounLinkerTrainer/PronounLinkerTrainer.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <iostream>

#include "Generic/common/version.h"

int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cerr << "\nTrainer usage: param_file\n";
		return -1;
	}
	try {
		ParamReader::readParamFile(argv[1]);

		std::string model_prefix = ParamReader::getRequiredParam("pronlink_model");
		std::string log_file = model_prefix + ".log";
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(), 0, 0);
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "This is PronounLinkerTrainer for the ACE task, version 1.00.\n"
							   << "Serif version: " << SerifVersion::getVersionString() << "\n"
							   << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
							   << "\n";
		*SessionLogger::logger << "_________________________________________________\n"
							   << "Starting PronounLinkerTrainer Session\n"
							   << "Parameters:\n";

		ParamReader::logParams();
		*SessionLogger::logger << "\n";

		PronounLinkerTrainer trainer(PronounLinkerTrainer::TRAIN);
		trainer.train();

		ParamReader::finalize();

		std::cerr << std::endl;
		SessionLogger::logger->displaySummary();
		std::cerr << std::endl;

		delete SessionLogger::logger;

	} catch (UnrecoverableException &e) {

		e.putMessage(std::cerr);
		return -1;
	}


	return 0;
}
