#ifndef MICLUSTER_H
#define MICLUSTER_H

// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

#include "MIClassTable.h"

using namespace std;

class MICluster
{
public:
	MICluster();
	~MICluster();
	
	void loadVocabulary(vector <wstring> elements);
	void doClusters(string bitsFile); 
	void loadBigram(int hist, int fut, int count);

private:
	MIClassTable t;

};


#endif
