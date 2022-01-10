// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/Cbrne/reader/en_CBRNESimpleDocumentReader.h"
#include "Generic/theories/Document.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Symbol.h"
#include <wchar.h>
#include <iostream>
#include <stdio.h>


using namespace std;

class DefaultDocumentReader;

CBRNESimpleDocumentReader::CBRNESimpleDocumentReader() {
}

CBRNESimpleDocumentReader::~CBRNESimpleDocumentReader() {
}

Document* CBRNESimpleDocumentReader::readDocument(InputStream &stream) {
	throw _new InternalInconsistencyException("CBRNESimpleDocumentReader::readDocument()", "This function is not implemented!");
	return NULL;
}


void CBRNESimpleDocumentReader::cleanRegion(LocatedString *region, Symbol inputType) {
	throw _new InternalInconsistencyException("CBRNESimpleDocumentReader::cleanRegion()", "This function is not implemented!");
}
