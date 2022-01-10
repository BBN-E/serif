// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_CHART_ENTRY_H
#define ar_CHART_ENTRY_H

#include <cstddef>
#include "Generic/common/Symbol.h"
#include "Generic/parse/BridgeType.h"
#include "Generic/parse/BridgeKernel.h"
#include "Generic/parse/BridgeExtension.h"
#include "Generic/parse/SequentialBigrams.h"
#include "Generic/parse/SignificantConstitNode.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ChartDecoder.h"

#define CHART_ENTRY_MAX_CHAIN 10
#define CHART_ENTRY_BLOCK_SIZE 10000

class ArabicChartEntry : public ChartEntry{
public:
	Symbol originalHeadWord;		//added for postprocessing the parse
	ParseNode* toParseNode();
	
	
};

#endif
