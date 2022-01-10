// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BRIDGE_EXTENSION_H
#define BRIDGE_EXTENSION_H

#include "Generic/common/Symbol.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"

struct BridgeExtension {
    int branchingDirection;
    Symbol constituentCategory;
    Symbol headCategory;
    Symbol previousModifierCategory;
    Symbol modifierBaseCategory;
    Symbol modifierChain;
    Symbol modifierChainFront;
    Symbol modifierTag;
};

UTF8InputStream& operator>>(UTF8InputStream& stream, BridgeExtension& extension)
    throw(UnexpectedInputException);
    
#endif

