// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CONN_CORPUS_STAT_H
#define CONN_CORPUS_STAT_H

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include "Generic/common/Symbol.h"

using namespace std;

class ConnectiveCorpusStatistics {
public:
	ConnectiveCorpusStatistics(){};
	ConnectiveCorpusStatistics(Symbol Name);
	~ConnectiveCorpusStatistics(){};

	void readOneSample (int label);
	Symbol getConnective (){return _connective;}
	int totalNegativeSamples(){return numOfNegativeSamples;}
	int totalPositiveSamples(){return numOfPositiveSamples;}
	int totalSamples(){return numOftotalSamples;}

private:
	Symbol _connective;
	int numOfNegativeSamples;
	int numOfPositiveSamples;
	int numOftotalSamples;

};

#endif
