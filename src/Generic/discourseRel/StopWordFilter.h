// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef STOP_WORD_FILTER_H
#define STOP_WORD_FILTER_H

#include <wchar.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string>
#include <map>
#include <iterator>
#include "Generic/common/Symbol.h"

using namespace std;

class StopWordFilter {
  public:
	  
	typedef map<string, int> StopWordDict;
	typedef map<string, int> FilteredWordDict;

	StopWordFilter();
	~StopWordFilter();
	
	static void loadStopWordDict (const char *filename);
	static int isInStopWordDict(string word);
	static void finalize ();
	static void initFilteredWordDict();
	static void showFilteredWords (UTF8OutputStream& stream);
	static void recordFilteredWords (string word);

  private:
	static int _num_words;
	static StopWordDict *_stopword_dict;
	static FilteredWordDict *_filteredword_dict;
};

#endif
