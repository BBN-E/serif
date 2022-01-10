// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EXTENSION_KEY_H
#define EXTENSION_KEY_H

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/UnexpectedInputException.h"

struct ExtensionKey {
    int branchingDirection;
    Symbol constituentCategory;
    Symbol headCategory;
    Symbol modifierBaseCategory;
    Symbol previousModifierCategory;
    Symbol modifierTag;
    ExtensionKey(int _branchingDirection, const Symbol &_constituentCategory,
        const Symbol &_headCategory, const Symbol &_modifierBaseCategory,
        const Symbol &_previousModifierCategory, const Symbol &_modifierTag) :
        branchingDirection(_branchingDirection),
        constituentCategory(_constituentCategory),
        headCategory(_headCategory),
        modifierBaseCategory(_modifierBaseCategory),
        previousModifierCategory(_previousModifierCategory),
        modifierTag(_modifierTag)
    {}
    ExtensionKey() {}
};

UTF8InputStream& operator>>(UTF8InputStream& stream, ExtensionKey& extensionKey)
    throw(UnexpectedInputException);
    
#endif
