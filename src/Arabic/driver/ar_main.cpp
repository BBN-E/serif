// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"

#include <iostream>

using namespace std;


int main (int argc, char *argv[]) {

	if (argc != 2) {
		cerr << "ArabicSerif.exe sould be invoked with a single argument, which provides a\n"
			 << "path to the parameter file.\n";
		return -1;
	}

	ParamReader::readParamFile(argv[1]);

	cout << "So far so good....\n";

	return 0;
}
