// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstring>
#include "Generic/parse/ExtensionKey.h"
#include "Generic/parse/BranchDirection.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/parse/ParserTags.h"
#include "Generic/common/UnexpectedInputException.h"

UTF8InputStream& operator>>(UTF8InputStream& stream, ExtensionKey& extensionKey)
    throw(UnexpectedInputException)
{
    UTF8Token token;

    stream >> token;
    if (stream.eof()) return stream;
    if (token.symValue() != ParserTags::leftParen)
		throw UnexpectedInputException("ExtensionKey::>>","ERROR: ill-formed extension key record");

    stream >> token;
    if (token.symValue() == ParserTags::LEFT)
        extensionKey.branchingDirection = BRANCH_DIRECTION_LEFT;
    else if (token.symValue() == ParserTags::RIGHT)
        extensionKey.branchingDirection = BRANCH_DIRECTION_RIGHT;
    else
        throw UnexpectedInputException("ExtensionKey::>>","ERROR: ill-formed extension key record");

    stream >> token;
    extensionKey.constituentCategory = token.symValue();

    stream >> token;
    extensionKey.headCategory = token.symValue();

    stream >> token;
    extensionKey.modifierBaseCategory = token.symValue();

    stream >> token;
    extensionKey.previousModifierCategory = token.symValue();

    stream >> token;
    extensionKey.modifierTag = token.symValue();

    stream >> token;
    if (token.symValue() != ParserTags::rightParen)
        throw UnexpectedInputException("ExtensionKey::>>","ERROR: ill-formed extension key record");

    return stream;
}
