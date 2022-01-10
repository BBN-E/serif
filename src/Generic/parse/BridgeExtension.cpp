// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include "Generic/parse/BridgeExtension.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

UTF8InputStream& operator>>(UTF8InputStream& stream, BridgeExtension& extension)
    throw(UnexpectedInputException)
{
    UTF8Token token;

    stream >> token;
    if (stream.eof()) return stream;
    if (token.symValue() != ParserTags::leftParen)
        throw UnexpectedInputException("BridgeExtension::>>","ERROR: ill-formed bridge extension record");

    stream >> token;
    if (token.symValue() == ParserTags::LEFT)
        extension.branchingDirection = BRANCH_DIRECTION_LEFT;
    else if (token.symValue() == ParserTags::RIGHT)
        extension.branchingDirection = BRANCH_DIRECTION_RIGHT;
    else
        throw UnexpectedInputException("BridgeExtension::>>","ERROR: ill-formed bridge extension record");

    stream >> token;
    extension.constituentCategory = token.symValue();

    stream >> token;
    extension.headCategory = token.symValue();

    stream >> token;
    extension.previousModifierCategory = token.symValue();

    stream >> token;
    extension.modifierBaseCategory = token.symValue();

    stream >> token;
    extension.modifierChain = token.symValue();

    stream >> token;
    extension.modifierChainFront = token.symValue();

    stream >> token;
    extension.modifierTag = token.symValue();

    stream >> token;
    if (token.symValue() != ParserTags::rightParen)
        throw UnexpectedInputException("BridgeExtension::>>","ERROR: ill-formed bridge extension record");

    return stream;
}
