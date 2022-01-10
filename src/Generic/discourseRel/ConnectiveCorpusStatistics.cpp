// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/discourseRel/ConnectiveCorpusStatistics.h"


ConnectiveCorpusStatistics::ConnectiveCorpusStatistics (Symbol name){
	_connective = name;
	numOfPositiveSamples=0;
	numOfNegativeSamples=0;
	numOftotalSamples=0;
}


void ConnectiveCorpusStatistics::readOneSample (int label){
	if (label == 1){
		numOfPositiveSamples++;
	}else if (label == 0){
		numOfNegativeSamples ++;
	}
	numOftotalSamples++;
}
