// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ExtensionTable.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

const float ExtensionTable::targetLoadingFactor = static_cast<float>(0.7);

ExtensionTable::ExtensionTable(UTF8InputStream& in)
{
    int numRecords;
    int numBuckets;
    ExtensionKey key;
    int listLength;
    UTF8Token token;

    in >> numRecords;
    numBuckets = static_cast<int>(numRecords / targetLoadingFactor);
    table = _new Table(numBuckets);
    for (int i = 0; i < numRecords; i++) {
        in >> token;
        if (token.symValue() != ParserTags::leftParen)
			throw UnexpectedInputException("ExtensionTable::()", "ERROR: ill-formed extension list");

        in >> key;
        in >> listLength;
        BridgeExtension* extensions = _new BridgeExtension[listLength];
        for (int j = 0; j < listLength; j++) {
            in >> extensions[j];
        }
        (*table)[key] = ExtensionList(listLength, extensions);

        in >> token;
        if (token.symValue() != ParserTags::rightParen)
            throw UnexpectedInputException("ExtensionTable::()", "ERROR: ill-formed extension list");
    }
}

ExtensionTable::~ExtensionTable() {
	if (table) {
		Table::iterator iter;
		for (iter = table->begin(); iter != table->end(); ++iter) {
			ExtensionList list = (*iter).second;
			delete[] list.data;
		}
		delete table;
	}
}
