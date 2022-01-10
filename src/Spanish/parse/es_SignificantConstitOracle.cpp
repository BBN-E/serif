// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Spanish/parse/es_SignificantConstitOracle.h"

SpanishSignificantConstitOracle::SpanishSignificantConstitOracle(KernelTable* kernelTable, 
												   ExtensionTable* extensionTable,
												   const char *inventoryFilename) 
{
}

bool SpanishSignificantConstitOracle::isSignificant(ChartEntry *entry, Symbol chain) const
{
	return false;
}

bool SpanishSignificantConstitOracle::isPossibleDescriptorHeadWord(Symbol word) const {
	return false;
}


bool SpanishSignificantConstitOracle::hasSignificantPP(ChartEntry *entry) const
{
		ChartEntry *entryIterator = entry;
		while (entryIterator->leftChild != 0) {
			if (entryIterator->bridgeType == BRIDGE_TYPE_KERNEL &&
				  entryIterator->kernelOp->branchingDirection == BRANCH_DIRECTION_LEFT)
				return false;
			if (entryIterator->bridgeType == BRIDGE_TYPE_EXTENSION &&
				  entryIterator->extensionOp->branchingDirection == BRANCH_DIRECTION_LEFT)
				return false;
			if (entryIterator->rightChild->isPPofSignificantConstit)
				return true;
			entryIterator = entryIterator->leftChild;
		}

		return false;	

}


