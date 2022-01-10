// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"
#include "ParameterStruct.h"


void ParameterStruct::initialize() {
	char source[] = "ParameterStruct::initialize()";
	// FILES OF RELATION INSTANCES
	getWideStringParam("maxent_sal_training_file_list", _training_filelist, PARAM_LENGTH, source);
	getWideStringParam("maxent_sal_activelearning_file_list", _active_pool_filelist, PARAM_LENGTH, source);
	getWideStringParam("maxent_sal_devtest_file_list", _devtest_filelist, PARAM_LENGTH, source);

	// MODEL FILE
	getStringParam("maxent_relation_model_file", _model_file, PARAM_LENGTH, source);

	// HIGH YIELD ANNOTATION FILE FOR OUTPUT
	getStringParam("maxent_sal_annotation_file", _annotation_ready_file, PARAM_LENGTH, source);

	// MAX PERCENTAGE OF NONE RELATIONS
	try {
		_max_none_percent = getIntParam("maxent_sal_max_none_percentage", source);
	} catch (...) {
		_max_none_percent = 100;
	}

	// BEAM WIDTH-- MUST be the same in devtest as in training
	try {
		_beam_width = getIntParam("beam_width", source);
	} catch (...) {
		_beam_width = 1;
	}

	char debug[PARAM_LENGTH];
	if (ParamReader::getParam("maxent_sal_debug",debug, PARAM_LENGTH) && strcmp(debug, "true") == 0) {
		DEBUG = true;
		
		// DEBUG FILE
		try {
			getStringParam("maxent_sal_debug_out", _debug_stream_file, PARAM_LENGTH, source);
		} catch (...) {
			sprintf(_debug_stream_file, "debug");
		}
	}

	char devtest[PARAM_LENGTH];
	if (ParamReader::getParam("maxent_sal_devtest",devtest, PARAM_LENGTH) && strcmp(devtest, "true") == 0) {

			// OUTPUT FILE
			try {
				getStringParam("maxent_sal_devtest_out", _output_file, PARAM_LENGTH, source);
			} catch (...) {
				sprintf(_output_file, "devtest");
			}

			// MODEL NUMBER
			try {
				_model_num = getIntParam("maxent_sal_starting_modelnum", source);
			} catch (...) {
			}

		} else { // TRAINING ONLY

			// TRAIN MODE
			char param_mode[10];
			getStringParam("maxent_trainer_mode", param_mode, 10, source);
			if (strcmp(param_mode, "GIS") == 0)
				_mode = MaxEntModel::GIS;
			else if (strcmp(param_mode, "SCGIS") == 0)
				_mode = MaxEntModel::SCGIS;
			else
				throw UnexpectedInputException(source,
				"Invalid setting for parameter 'maxent_trainer_mode'");

			// PRUNING
			_pruning = getIntParam("maxent_trainer_pruning_cutoff", source);

			// PERCENT HELD OUT
			_percent_held_out = getIntParam("maxent_trainer_percent_held_out", source);
			if (_percent_held_out < 0 || _percent_held_out > 50)
				throw UnexpectedInputException(source,
				"Parameter 'maxent_trainer_percent_held_out' must be between 0 and 50");

			// WEIGHTSUM GRANULARITY
			_weightsum_granularity = getIntParam("maxent_trainer_weightsum_granularity", source);

			// GAUSSIAN PRIOR VARIANCE
			_variance = getDoubleParam("maxent_trainer_gaussian_variance", source);

			// MIN CHANGE IN LIKELIHOOD (STOPPING CONDITION)
			try {
				_likelihood_delta = getDoubleParam("maxent_min_likelihood_delta", source);
			} catch (...) {
				_likelihood_delta = .0001;
			}

			// FREQUENCY OF STOPPING CONDITION CHECKS (NUM ITERATIONS)
			try {
				_stop_check_freq = getIntParam("maxent_stop_check_frequency", source);
			} catch (...) {
				_stop_check_freq = 1;
			}

			// MAX NUMBER OF MAXENT ITERATIONS
			try {
				_max_iterations = getIntParam("maxent_trainer_n_iterations", source);
			} catch (...) {
				_max_iterations = 1000;
			}

			// NUMBER OF ACTIVE LEARNING ITERATIONS
			try {
				_numALIterations = getIntParam("maxent_sal_num_active_iterations", source);
			} catch (...) {
				_numALIterations = 1;
			}

			// ACTIVE LEARNING SENTENCES TO ASK FOR IN EACH ITERATION
			_numActiveToAdd = getIntParam("maxent_sal_active_increment", source);

			// NUMBER OF TRAINING SENTENCES TO START WITH
			// this lets us specify a large file but not use all of it
			try {
				_numTrainingToAdd = getIntParam("maxent_sal_num_starting_instances", source);
			} catch (...) {
				_numTrainingToAdd = -1;  // will use everything in file list
			}

			// VECTOR FILES
			try {
				getStringParam("maxent_train_vector_file", _train_vector_file, PARAM_LENGTH, source);
			} catch (...) {
				_train_vector_file[0] = '\0';
			}

			try {
				getStringParam("maxent_test_vector_file", _test_vector_file, PARAM_LENGTH, source);
			} catch (...) {
				_test_vector_file[0] = '\0';
			}
		}
}

void ParameterStruct::printParameters(std::ostream& out) const {
	char line[] = "======================================================\n";
	out << "\n";
	out << line;
	out << "PARAMETERS\n";
	out << "mode: " << ((_mode == MaxEntModel::SCGIS) ? "SCGIS" : "GIS")
		<< "\tpruning: " << _pruning << "\tbeam width: " << _beam_width << "\n";
	out << "likelihood delta: " << _likelihood_delta << "\tstop check frequency: " << _stop_check_freq
		<< "\tvariance: " << _variance << "\n";
	out << "held out: " << _percent_held_out << "%\tgranularity: " << _weightsum_granularity
		<< "\tmax iterations: " << _max_iterations << "\n";
	out << "initial training: " << _numTrainingToAdd << "\tAL iterations: " << _numALIterations
		<< "\tinstances per iteration: " << _numActiveToAdd << "\n";
	out << "max NONE relations: " << _max_none_percent << "%" << "\n";
	printf("initial training filelist: %S\n", _training_filelist);
	printf("active learning pool filelist: %S\n", _active_pool_filelist);
	out << "model file: " << _model_file << "\n";
	printf("devtest filelist: %S\n", _devtest_filelist);
	out << "devtest output file: " << _output_file << "\n";
	out << "annotation output file: " << _annotation_ready_file << "\n";
	out << line;
}
