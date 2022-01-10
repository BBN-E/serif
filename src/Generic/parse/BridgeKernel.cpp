// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include "Generic/parse/BridgeKernel.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

UTF8InputStream& operator>>(UTF8InputStream& stream, BridgeKernel& kernel)
    throw(UnexpectedInputException)
{
    UTF8Token token;

    stream >> token;
    if (stream.eof()) return stream;
    if (token.symValue() != ParserTags::leftParen)
        throw UnexpectedInputException("BridgeKernel::>>","ERROR: ill-formed bridge kernel record");

    stream >> token;
    if (token.symValue() == ParserTags::LEFT)
        kernel.branchingDirection = BRANCH_DIRECTION_LEFT;
    else if (token.symValue() == ParserTags::RIGHT)
        kernel.branchingDirection = BRANCH_DIRECTION_RIGHT;
    else
        throw UnexpectedInputException("BridgeKernel::>>","ERROR: ill-formed bridge kernel record");

    stream >> token;
    kernel.constituentCategory = token.symValue();

    stream >> token;
    kernel.headBaseCategory = token.symValue();

    stream >> token;
    kernel.modifierBaseCategory = token.symValue();

    stream >> token;
    kernel.headChain = token.symValue();

    stream >> token;
    kernel.headChainFront = token.symValue();

    stream >> token;
    kernel.modifierChain = token.symValue();

    stream >> token;
    kernel.modifierChainFront = token.symValue();

    stream >> token;
    kernel.modifierTag = token.symValue();

    stream >> token;
    if (token.symValue() != ParserTags::rightParen)
        throw UnexpectedInputException("BridgeKernel::>>","ERROR: ill-formed bridge kernel record");

    return stream;
}



