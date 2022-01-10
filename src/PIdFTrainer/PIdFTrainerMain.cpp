// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#if defined(WIN32) || defined(WIN64)
#include <crtdbg.h>
#include <direct.h>
#include <sys/stat.h>
#endif

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FeatureModule.h"
#include "Generic/names/discmodel/PIdFModel.h"
#include "Generic/names/discmodel/PIdFActiveLearning.h"
#include "Generic/names/IdFWordFeatures.h"

#include "dynamic_includes/common/ProfilingDefinition.h"

#ifdef DO_SERIF_PROFILING
#include "Generic/common/GenericTimer.h"

GenericTimer totalLoadTimer;
GenericTimer totalProcessTimer;
#endif


using namespace std;

int main(int argc, char **argv) {

	if (argc != 2) {
		cerr << "PIdfTrainer.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		#ifdef DO_SERIF_PROFILING
			totalLoadTimer.startTimer();
		#endif

		ParamReader::readParamFile(argv[1]);
		FeatureModule::load();
		char feature_mode[500];
		bool timex_mode = false;
		bool other_value_mode = false;
		if (ParamReader::getParam("pidf_word_features_mode", feature_mode, 500)) {
			if (strcmp(feature_mode, "TIMEX") == 0)
				timex_mode = true;
			else if (strcmp(feature_mode, "OTHER_VALUE") == 0)
				other_value_mode = true;
		}

		char emptyModelFileSuffix[1];
		emptyModelFileSuffix[0] = '\0';
		char buffer[500];
		if (ParamReader::getParam("pidf_standalone_mode", buffer, 500)) {
			if (strcmp(buffer, "train") == 0) {
	
				// Create the parent directory for our model if we need to
				char model_file_buffer[4096];
				if (ParamReader::getParam("pidf_model_file", model_file_buffer, 4096)) {
					string model_file_string(model_file_buffer);
				    size_t index = model_file_string.find_last_of(SERIF_PATH_SEP);
				    _mkdir(model_file_string.substr(0, index).c_str());
				}
				PIdFModel *trainer = _new PIdFModel(PIdFModel::TRAIN);
				
				if (timex_mode)
					trainer->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				else if (other_value_mode)
					trainer->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);
				std::string training_file = ParamReader::getParam("pidf_training_file");
				if (training_file != "") 
					trainer->addTrainingSentencesFromTrainingFile(training_file.c_str(), false);
				std::string encrypted_training_file = ParamReader::getParam("pidf_encrypted_training_file");
				if (encrypted_training_file != "") 
					trainer->addTrainingSentencesFromTrainingFile(encrypted_training_file.c_str(), true);
				if (training_file == "" && encrypted_training_file == "") {
					throw UnexpectedInputException("PIdFTrainerMain::main()",
						"At least one of 'pidf_training_file' or 'pidf_encrypted_training_file' must be specified.");
				}

				#ifdef DO_SERIF_PROFILING
					totalLoadTimer.stopTimer();
					totalProcessTimer.startTimer();
				#endif
				
				trainer->train();

				#ifdef DO_SERIF_PROFILING
					totalProcessTimer.stopTimer();
				#endif

				trainer->writeModel(emptyModelFileSuffix);
				delete trainer;
			}
			else if (strcmp(buffer, "train_and_decode") == 0) {
				PIdFModel *trainer = _new PIdFModel(PIdFModel::TRAIN);
				if (timex_mode)
					trainer->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				else if (other_value_mode)
					trainer->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);
				std::string training_file = ParamReader::getParam("pidf_training_file");
				if (training_file != "") 
					trainer->addTrainingSentencesFromTrainingFile(training_file.c_str(), false);
				std::string encrypted_training_file = ParamReader::getParam("pidf_encrypted_training_file");
				if (encrypted_training_file != "") 
					trainer->addTrainingSentencesFromTrainingFile(encrypted_training_file.c_str(), true);
				if (training_file == "" && encrypted_training_file == "") {
					throw UnexpectedInputException("PIdFTrainerMain::main()",
						"At least one of 'pidf_training_file' or 'pidf_encrypted_training_file' must be specified.");
				}
				trainer->train();
				trainer->writeModel(emptyModelFileSuffix);
				trainer->finalizeWeights();
				delete trainer;

				PIdFModel *decoder = _new PIdFModel(PIdFModel::DECODE);
				decoder->decode();
				delete decoder;
			}
			else if (strcmp(buffer, "decode") == 0) {
				PIdFModel *decoder = _new PIdFModel(PIdFModel::DECODE);
				if (timex_mode)
					decoder->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				else if (other_value_mode)
					decoder->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);

				#ifdef DO_SERIF_PROFILING
					totalLoadTimer.stopTimer();
					totalProcessTimer.startTimer();
				#endif

				decoder->decode();

				#ifdef DO_SERIF_PROFILING
					totalProcessTimer.stopTimer();
				#endif

				delete decoder;
			}
            else if (strcmp(buffer, "decode_and_choose") == 0) {
				PIdFModel *decoder = _new PIdFModel(PIdFModel::DECODE_AND_CHOOSE);
				if (timex_mode)
					decoder->setWordFeaturesMode(IdFWordFeatures::TIMEX);
				else if (other_value_mode)
					decoder->setWordFeaturesMode(IdFWordFeatures::OTHER_VALUE);

				#ifdef DO_SERIF_PROFILING
					totalLoadTimer.stopTimer();
					totalProcessTimer.startTimer();
				#endif

				decoder->decode();
                decoder->addActiveLearningSentencesFromTrainingFile();
                decoder->chooseHighScoreSentences();
                decoder->writeSentences(emptyModelFileSuffix);

				#ifdef DO_SERIF_PROFILING
					totalProcessTimer.stopTimer();
				#endif

				delete decoder;
            }              
			else if (strcmp(buffer, "convert_ql_training") == 0) {
				PIdFActiveLearning *activeLearner = _new PIdFActiveLearning();
				activeLearner->Initialize(argv[1]);
				activeLearner->writePIdFTrainingFile();
			}
			else if (strcmp(buffer, "sim_al") == 0) {
                PIdFModel *altrainer = _new PIdFModel(PIdFModel::SIM_AL);
				
				altrainer->addActiveLearningSentencesFromTrainingFile();
				altrainer->seedALTraining();
				altrainer->doActiveLearning();
				altrainer->writeModel(emptyModelFileSuffix);
				//altrainer->freeWeights();
				delete altrainer;
			}
			else if (strcmp(buffer, "unsup") == 0) {
				PIdFModel *bagger = _new PIdFModel(PIdFModel::UNSUP);
				bagger->addActiveLearningSentencesFromTrainingFile();
				bagger->seedALTraining();
				bagger->doUnsupervisedTrainAndSelect();
				delete bagger;
			}
			else if (strcmp(buffer, "memtest") == 0) {
				PIdFModel** testers = _new PIdFModel*[100];
				for (int i = 0; i < 100; i++) {
					std::cout << "initialize " << i << std::endl;
					for( int j = 0; j < 100; j++) {
						testers[j] = _new PIdFModel(PIdFModel::UNSUP_CHILD);
					}
					std::cout << "delete " << i << std::endl;
					for (int k = 0; k < 100; k++) {
						testers[k]->freeWeights();
						delete testers[k];
					}
				}
			}
			else {
				throw UnexpectedInputException("PIdFTrainerMain::main()",
					"Parameter 'pidf_standalone_mode' invalid");
			}
		}
		else {
			throw UnexpectedInputException("PIdFTrainerMain::main()",
				"Parameter 'pidf_standalone_mode' not specified");
		}

		#ifdef DO_SERIF_PROFILING
            cout << "Load time\t" << totalLoadTimer.getTime() << " msec" << endl;
            cout << "Processing time\t" << totalProcessTimer.getTime() << " msec" << endl; 
		#endif
	}
	catch (UnrecoverableException &e) {
		cerr << "\n" << e.getSource() << " " << e.getMessage() << "\n";
		//HeapChecker::checkHeap("main(); About to exit due to error");

#ifdef _DEBUG
		cerr << "Press enter to exit....\n";
		getchar();
#endif

		return -1;
	}

	//HeapChecker::checkHeap("main(); About to exit after successful run");
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
