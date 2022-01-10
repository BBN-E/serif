// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_SignificantConstitOracle.h"


EnglishSignificantConstitOracle::EnglishSignificantConstitOracle(KernelTable* kernelTable, 
												   ExtensionTable* extensionTable,
												   const char *inventoryFile) 
: descOracle(_new EnglishDescOracle(kernelTable, extensionTable, inventoryFile)),
  nameOracle(_new EnglishNameOracle())
	{}

bool EnglishSignificantConstitOracle::isSignificant(ChartEntry *entry, Symbol chain) const
{
	if ((!entry->isPreterminal || chain != entry->headTag) && entry->headIsSignificant)
		return true;
	if (nameOracle->isName(entry, chain))
		return true;
	if (descOracle->isNounPhrase(entry, chain)) {
		if (hasSignificantPP(entry))
			return true;
		if (descOracle->isPartitive(entry))
			return true;
	}
	if (descOracle->isPossibleDescriptor(entry, chain))
		return true;
	return false;

}

bool EnglishSignificantConstitOracle::hasSignificantPP(ChartEntry *entry) const
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


