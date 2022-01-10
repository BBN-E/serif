// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BRIDGE_KERNEL_H
#define BRIDGE_KERNEL_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"


struct BridgeKernel {
    int branchingDirection;
    Symbol constituentCategory;
    Symbol headBaseCategory;
    Symbol modifierBaseCategory;
    Symbol headChain;
    Symbol headChainFront;
    Symbol modifierChain;
    Symbol modifierChainFront;
    Symbol modifierTag;
};

UTF8InputStream& operator>>(UTF8InputStream& stream, BridgeKernel& kernel)
    throw(UnexpectedInputException);
    
#endif

