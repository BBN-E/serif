// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// PPartOfSpeechTrainer.cpp : Defines the entry point for the console application.
//


#include "common/leak_detection.h"

//#include <stdio.h>
//#include <crtdbg.h>
#include <string>

#include "common/UnrecoverableException.h"
#include "common/ParamReader.h"
#include "common/HeapChecker.h"
#include "partOfSpeech/discmodel/PPartOfSpeechModel.h"


using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "PPartOfSpeechTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}


	char emptyModelFileSuffix[1];
	emptyModelFileSuffix[0] = '\0';
	try {
		ParamReader::readParamFile(argv[1]);

		string mode = ParamReader::getRequiredParam("pidf_trainer_standalone_mode");

		if (mode == "train") {
			PPartOfSpeechModel *trainer = _new PPartOfSpeechModel(PPartOfSpeechModel::TRAIN);
          
	        std:: string training_file = ParamReader::getParam("pidf_training_file");
		    trainer->addTrainingSentencesFromTrainingFile(training_file.c_str(), false);
			trainer->train();
			trainer->writeModel(emptyModelFileSuffix);
            delete trainer;
		}
		else if (mode == "decode") {
			string modelFile = ParamReader::getRequiredParam("pidf_model_file");
			PPartOfSpeechModel *decoder = _new PPartOfSpeechModel(PPartOfSpeechModel::DECODE);
			decoder->decode();
            delete decoder;
		}
        else if (mode == "sim_al") {
          string modelFile = ParamReader::getRequiredParam("pidf_model_file");
          PPartOfSpeechModel * altrainer = _new PPartOfSpeechModel(PPartOfSpeechModel::SIM_AL);
          altrainer->addActiveLearningSentencesFromTrainingFile();
          altrainer->seedALTraining();
          altrainer->doActiveLearning();
          altrainer->writeModel(emptyModelFileSuffix);
          delete altrainer;
        }
		else {
			throw UnexpectedInputException("PPartOfSpeechTrainer::main()",
				"Parameter 'pidf_trainer_standalone_mode' invalid");
		}
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
		HeapChecker::checkHeap("main(); About to exit due to error");

#ifdef _DEBUG
		cerr << "Press enter to exit....\n";
		getchar();
#endif
		return -1;
	}

	HeapChecker::checkHeap("main(); About to exit after successful run");

#if 0
	ParamReader::finalize();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtDumpMemoryLeaks();
#endif

#ifdef _DEBUG
	cerr << "Press enter to exit....\n";
	getchar();
#endif

	return 0;
}
