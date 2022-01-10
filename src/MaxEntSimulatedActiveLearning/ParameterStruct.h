// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARAMETER_STRUCT_H
#define PARAMETER_STRUCT_H

#include "maxent/MaxEntModel.h"
#include "Utilities.h"

class ParameterStruct {
public:
	ParameterStruct()
		: _mode(MaxEntModel::SCGIS), _model_num(-1), _numActiveToAdd(50), _numTrainingToAdd(-1), _numALIterations(1),
		_weightsum_granularity(0), _pruning(0), _beam_width(1), _percent_held_out(0),
		_max_none_percent(100), _likelihood_delta(.0001), _stop_check_freq(1), _max_iterations(1000),
		_variance(0), DEBUG(false)
	{
		wcsncpy(_training_filelist, L"filenotset.training", PARAM_LENGTH);
		wcsncpy(_active_pool_filelist, L"filenotset.activepool", PARAM_LENGTH);
		wcsncpy(_devtest_filelist, L"filenotset.devtest", PARAM_LENGTH);
		strncpy(_model_file, "filenotset.model", PARAM_LENGTH);
		strncpy(_train_vector_file, "", PARAM_LENGTH);
		strncpy(_test_vector_file, "", PARAM_LENGTH);
		strncpy(_output_file, "filenotset.output", PARAM_LENGTH);
		strncpy(_annotation_ready_file, "filenotset.anno", PARAM_LENGTH);
		strncpy(_debug_stream_file, "filenotset.debug", PARAM_LENGTH);
	}

	void initialize();  // reads in values from .par file
	void printParameters(std::ostream& out) const;

	int _mode;

	wchar_t _training_filelist[PARAM_LENGTH];
	wchar_t _active_pool_filelist[PARAM_LENGTH];
	wchar_t _devtest_filelist[PARAM_LENGTH];


	char _model_file[PARAM_LENGTH];	
	char _train_vector_file[PARAM_LENGTH];
	char _test_vector_file[PARAM_LENGTH];
	char _annotation_ready_file[PARAM_LENGTH]; // FIXME
	char _debug_stream_file[PARAM_LENGTH];

	bool DEBUG;

	// for devtest
	char _output_file[PARAM_LENGTH];
	int _model_num;

	int _numActiveToAdd;   // per iteration
	int _numTrainingToAdd;

	int _numALIterations; 

	int _weightsum_granularity;
	int _pruning;
	int _beam_width;
	int _percent_held_out;
	int _max_none_percent;  // at most this % of the training data can be NONE relations
	double _likelihood_delta;
	int _stop_check_freq;
	int _max_iterations;
	double _variance;
};

#endif
