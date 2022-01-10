// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KERNEL_KEY_H
#define KERNEL_KEY_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/UnexpectedInputException.h"

struct KernelKey {
    int branchingDirection;
    Symbol headBaseCategory;
    Symbol modifierBaseCategory;
    Symbol modifierTag;
    KernelKey(int _branchingDirection, const Symbol &_headBaseCategory,
        const Symbol &_modifierBaseCategory, const Symbol &_modifierTag) :
        branchingDirection(_branchingDirection),
        headBaseCategory(_headBaseCategory),
        modifierBaseCategory(_modifierBaseCategory),
        modifierTag(_modifierTag)
    {}
    KernelKey() {}
};

UTF8InputStream& operator>>(UTF8InputStream& stream, KernelKey& kernelKey)
    throw(UnexpectedInputException);
    
#endif
