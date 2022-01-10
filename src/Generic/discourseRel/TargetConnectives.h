// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TARGET_CONN_H
#define TARGET_CONN_H

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

class TargetConnectives {
  public:
	  
	typedef map<string, int> ExplicitConnectiveDict;

	TargetConnectives();
	~TargetConnectives();
	
	static void loadConnDict (const char *filename);
	static int isInConnDict(string word);
	static TargetConnectives::ExplicitConnectiveDict* getConnDict(){return _connective_dict;}
	static void finalize ();

  private:
	static int _num_words;
	static ExplicitConnectiveDict *_connective_dict;

};

#endif
