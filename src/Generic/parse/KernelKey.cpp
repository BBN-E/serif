// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include "Generic/parse/KernelKey.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

UTF8InputStream& operator>>(UTF8InputStream& stream, KernelKey& kernelKey)
    throw(UnexpectedInputException)
{
    UTF8Token token;

    stream >> token;
    if (stream.eof()) return stream;
    if (token.symValue() != ParserTags::leftParen)
		throw UnexpectedInputException("KernelKey::>>","ERROR: ill-formed kernel key record");

    stream >> token;
    if (token.symValue() == ParserTags::LEFT)
        kernelKey.branchingDirection = BRANCH_DIRECTION_LEFT;
    else if (token.symValue() == ParserTags::RIGHT)
        kernelKey.branchingDirection = BRANCH_DIRECTION_RIGHT;
    else
        throw UnexpectedInputException("KernelKey::>>","ERROR: ill-formed kernel key record");

    stream >> token;
    kernelKey.headBaseCategory = token.symValue();

    stream >> token;
    kernelKey.modifierBaseCategory = token.symValue();

    stream >> token;
    kernelKey.modifierTag = token.symValue();

    stream >> token;
    if (token.symValue() != ParserTags::rightParen)
        throw UnexpectedInputException("KernelKey::>>","ERROR: ill-formed kernel key record");

    return stream;
}

