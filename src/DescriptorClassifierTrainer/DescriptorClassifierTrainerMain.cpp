// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/descriptors/DescriptorClassifierTrainer/DescriptorClassifierTrainer.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/driver/SessionProgram.h"
#include <iostream>

#include "Generic/common/version.h"

int main(int argc, char* argv[]) {

	if (argc != 2) {
		std::cerr << "\nTrainer usage: param_file\n";
		return -1;
	}
	try {
		ParamReader::readParamFile(argv[1]);

		char model_prefix[501];
		char log_file[501];

		if (!ParamReader::getParam("desc_classify_model",model_prefix,									 500))		{
			throw UnexpectedInputException("DescriptorClassifierTrainerMain::main()",
									   "Param `desc_classify_model' not defined");
		}
		sprintf(log_file,"%s.log", model_prefix);


		SessionLogger::logger = _new FileSessionLogger(log_file, 0, 0);
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "This is DescriptorClassifierTrainer for the ACE task, version 1.00.\n"
							   << "Serif version: " << SerifVersion::getVersionString() << "\n"
							   << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
							   << "\n";
		*SessionLogger::logger << "_________________________________________________\n"
							   << "Starting DescriptorClassifierTrainer Session\n"
							   << "Parameters:\n";

		ParamReader::logParams();
		*SessionLogger::logger << "\n";

		DescriptorClassifierTrainer trainer;

		char file[501];
		char file_list[501];
		if (ParamReader::getParam("training_file",file,									 500))		{
			*SessionLogger::logger << "Training file:\n" << file << ".\n";
			trainer.openInputFile(file);
			trainer.trainModels();
			trainer.closeInputFile();
			trainer.writeModels();
		}
		else if (ParamReader::getParam("training_file_list",file_list,										500))		{
			std::basic_ifstream<char> in;
			in.open(file_list);
			if (in.fail()) {
				throw UnexpectedInputException(
					"DescriptorClassifierTrainerMain::main()",
					"could not open training file list");
			}

			*SessionLogger::logger << "Training files:\n";
			while (!in.eof()) {
				in.getline(file, 500);
				*SessionLogger::logger << file << ".\n";
				trainer.openInputFile(file);
				trainer.trainModels();
				trainer.closeInputFile();
			}
			trainer.writeModels();
			in.close();
		}
		else  {
			throw UnexpectedInputException("DescriptorClassifierTrainerMain::main()",
									   "Params `training_file' and 'training_file_list' both undefined");
		}


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
