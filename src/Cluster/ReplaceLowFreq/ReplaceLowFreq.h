#ifndef REPLACE_LOW_FREQ_H
#define REPLACE_LOW_FREQ_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#pragma once

#include <string>
#include <vector>
#include "CountTable.h"
//#include "theories/TokenSequence.h"

using namespace std;

class ReplaceLowFreq
{
public:
	ReplaceLowFreq();
	~ReplaceLowFreq();

	vector <wstring> doReplaceLowFreq(vector <wstring> tokens);
	void pruneToThreshold(int threshold, const char * rare_words_file = 0);
	//void addCounts(TokenSequence * tokenSeq);
	void addCounts(Symbol word);
private:
	CountTable * _countTable;
	vector <wstring> _result;
	
	void replaceLowFreqWords(vector <wstring> tokens);
};


#endif
