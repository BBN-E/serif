// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/relations/RelationModel.h"
#include "Generic/relations/RelationTypeSet.h"
#include <iostream>

int main(int argc, char* argv[]) {

	if (argc != 2 && argc != 4 && argc != 5 && argc != 6) {
		std::cerr << "\nTrainer usage: param_file [-v] vector_file output_file_prefix [test_file]\n";
		std::cerr << "Note: the -v (generate vectors and train) option is only implemented for Chinese\n";
		return -1;
	}
	try {

		ParamReader::readParamFile(argv[1]);
		RelationTypeSet::initialize();
		RelationModel *model = RelationModel::build();

		std::string log_file = ParamReader::getRequiredParam("relation_trainer_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());

		const wchar_t *context_name = L"relation-training";
		SessionLogger::logger = new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);

		if (argc == 2) {

			char training_file[501];
			char output_prefix[501];
			if (!ParamReader::getParam("relation_training_file",training_file, 500))				throw UnrecoverableException("RelationTrainer::main()",									 "Parameter 'relation_training_file' not specified");
			if (!ParamReader::getParam("relation_model_file",output_prefix, 500))				throw UnrecoverableException("RelationTrainer::main()",									 "Parameter 'relation_model_file' not specified");
			model->trainFromStateFileList(training_file, output_prefix);

		} else if (strcmp(argv[2], "-v") == 0) {
			char packet_file[501];
			char vector_file[501];
			if (!ParamReader::getParam("relation_packet_file",packet_file, 500))				throw UnrecoverableException("RelationTrainer::main()",									 "Parameter 'relation_packet_file' not specified");
			if (!ParamReader::getParam("relation_vector_file",vector_file, 500))				throw UnrecoverableException("RelationTrainer::main()",									 "Parameter 'relation_vector_file' not specified");
			model->generateVectorsAndTrain(packet_file, vector_file, argv[4]);
			if (argc == 6)
				model->test(argv[5]);
		}
		else {
			model->train(argv[2], argv[3]);
			if (argc == 5)
				model->test(argv[4]);
		}

	} catch (UnrecoverableException &e) {

		e.putMessage(std::cerr);
		return -1;
	}

	return 0;
}
