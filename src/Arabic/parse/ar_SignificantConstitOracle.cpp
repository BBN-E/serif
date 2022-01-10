// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_SignificantConstitOracle.h"

ArabicSignificantConstitOracle::ArabicSignificantConstitOracle(KernelTable* kernelTable, 
												   ExtensionTable* extensionTable,
												   const char *inventoryFilename) 
: descOracle(_new ArabicDescOracle(kernelTable, extensionTable)),
  nameOracle(_new ArabicNameOracle())
	{}

bool ArabicSignificantConstitOracle::isSignificant(ChartEntry *entry, Symbol chain) const
{

	
	if (nameOracle->isName(entry, chain))
		return true;
	if (descOracle->isPossibleDescriptor(entry, chain))
		return true;
	if ((!entry->isPreterminal || chain != entry->headTag) && entry->headIsSignificant)
		return true;
	if (descOracle->isNounPhrase(entry, chain)) {
		if (hasSignificantPP(entry))
			return true;
	}
	return false;

}
bool ArabicSignificantConstitOracle::hasSignificantPP(ChartEntry *entry) const
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
