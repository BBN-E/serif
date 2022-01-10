// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/RelationConstants.h"

Symbol RelationConstants::NONE = Symbol(L"NONE");
Symbol RelationConstants::RAW = Symbol(L"RAW");
Symbol RelationConstants::IDENTITY = Symbol(L"IDENTITY");
Symbol RelationConstants::ROLE_CITIZEN_OF = Symbol(L"ROLE.CITIZEN-OF");

RelationConstants::SymSymMap RelationConstants::_baseTypeSymbol;
RelationConstants::SymSymMap RelationConstants::_subtypeSymbol;
