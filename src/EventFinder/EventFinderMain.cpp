// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/FileSessionLogger.h"
#include "Generic/events/stat/StatEventTrainer.h"
#include "Generic/docRelationsEvents/StatEventLinker.h"
#include "Generic/common/FeatureModule.h"

using namespace std;


int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << "EventFinder.exe sould be invoked with a single argument, which provides a\n"
			<< "path to the parameter file.\n";
		return -1;
	}

	try {
		ParamReader::readParamFile(argv[1]);

		FeatureModule::load();

		std::string log_file = ParamReader::getRequiredParam("event_finder_log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());
		const wchar_t *context_name = L"event-training";
		SessionLogger::logger = _new FileSessionLogger(log_file_as_wstring.c_str(),
									   1, &context_name);


		char temp[501];
		bool train = false;
		bool round_robin = false;
		bool select_annotation = false;	
		if (ParamReader::getParam("event_training_file_list",temp, 500))			train = true;
		if (ParamReader::getParam("event_round_robin_setup",temp, 500))			round_robin = true;
		if (ParamReader::getParam("event_select_annotation",temp, 500))			select_annotation = true;
	
		if (ParamReader::getRequiredTrueFalseParam("train_event_trigger_model") ||
			ParamReader::getRequiredTrueFalseParam("train_event_aa_model") || 
			ParamReader::getRequiredTrueFalseParam("train_event_modality_model"))
		{
			if (train) {
				StatEventTrainer *trainer = _new StatEventTrainer(StatEventTrainer::TRAIN);
				trainer->train();
				delete trainer;
			}
			if (round_robin) {
				StatEventTrainer *trainer = _new StatEventTrainer(StatEventTrainer::ROUNDROBIN);
				trainer->roundRobin();
				delete trainer;
			}
			if (select_annotation) {
				StatEventTrainer *trainer = _new StatEventTrainer(StatEventTrainer::SELECT_ANNOTATION);
				trainer->selectAnnotation();
				delete trainer;
			}

		}

		// add devTest 2008-01-22
		char devtest[500];
		if (ParamReader::getNarrowParam(devtest, Symbol(L"devtest_event_modality_model"), 500))
		{
			if (strcmp(devtest, "true") == 0) {
				StatEventTrainer *trainer = _new StatEventTrainer(StatEventTrainer::DEVTEST);			
				trainer->devTest();
				delete trainer;
			}
		} 


		if (ParamReader::getRequiredTrueFalseParam("train_event_link_model"))
		{
			if (train) {
				StatEventLinker *trainer = _new StatEventLinker(StatEventLinker::TRAIN);
				trainer->train();
				delete trainer;
			}
			if (round_robin) {
				StatEventLinker *trainer = _new StatEventLinker(StatEventLinker::ROUNDROBIN);
				trainer->roundRobin();
				delete trainer;
			}
		}

		if(ParamReader::getOptionalTrueFalseParamWithDefaultVal("event_aa_devtest_using_gold_ev_mention", false)) {
			StatEventTrainer *trainer = _new StatEventTrainer(StatEventTrainer::AADEVTEST);
			trainer->eventAADevTestUsingGoldEVMentions();
			delete trainer;
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
