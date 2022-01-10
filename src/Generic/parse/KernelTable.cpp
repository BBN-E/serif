// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/KernelTable.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

const float KernelTable::targetLoadingFactor = static_cast<float>(0.7);

KernelTable::KernelTable(UTF8InputStream& in)
{
    int numRecords;
    int numBuckets;
    KernelKey key;
    int listLength;
    UTF8Token token;

    in >> numRecords;
    numBuckets = static_cast<int>(numRecords / targetLoadingFactor);
    table = _new Table(numBuckets);
    for (int i = 0; i < numRecords; i++) {
        in >> token;
        if (token.symValue() != ParserTags::leftParen)
            throw UnexpectedInputException("KernelTable::()","ERROR: ill-formed kernel list");

        in >> key;
        in >> listLength;
        BridgeKernel* kernels = _new BridgeKernel[listLength];
        for (int j = 0; j < listLength; j++) {
            in >> kernels[j];
        }
        (*table)[key] = KernelList(listLength, kernels);

        in >> token;
        if (token.symValue() != ParserTags::rightParen)
            throw UnexpectedInputException("KernelTable::()","ERROR: ill-formed kernel list");
    }
}

KernelTable::~KernelTable() {
	if (table) {
		Table::iterator iter;
		for (iter = table->begin(); iter != table->end(); ++iter) {
			KernelList kernelList = (*iter).second;
			delete[] kernelList.data;
		}
		delete table;
	}
}


