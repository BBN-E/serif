// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/CASerif/correctanswers/CASymbolicConstants.h"
#include "common/Symbol.h"

Symbol CASymbolicConstants::FALSE_SYM = Symbol(L"FALSE");
Symbol CASymbolicConstants::TRUE_SYM = Symbol(L"TRUE");
// CorrectMention mention types for ERE
Symbol CASymbolicConstants::NAME_ERE = Symbol(L"NAM");
Symbol CASymbolicConstants::NOMINAL_ERE = Symbol(L"NOM");
Symbol CASymbolicConstants::PRONOUN_ERE = Symbol(L"PRO");
Symbol CASymbolicConstants::NONE_ERE = Symbol(L"NA");

// CorrectMention mention types
Symbol CASymbolicConstants::NAME_LOWER = Symbol(L"Name");
Symbol CASymbolicConstants::NAME_UPPER = Symbol(L"NAME");
Symbol CASymbolicConstants::NOMINAL_LOWER = Symbol(L"Nominal");
Symbol CASymbolicConstants::NOMINAL_UPPER = Symbol(L"NOMINAL");
Symbol CASymbolicConstants::DATETIME_LOWER = Symbol(L"Date/Time");
Symbol CASymbolicConstants::DATETIME_UPPER = Symbol(L"DATE/TIME");
Symbol CASymbolicConstants::PRONOUN_LOWER = Symbol(L"Pronoun");
Symbol CASymbolicConstants::PRONOUN_UPPER = Symbol(L"PRONOUN");
Symbol CASymbolicConstants::PRE_UPPER = Symbol(L"PRE");
Symbol CASymbolicConstants::NOM_PRE_UPPER = Symbol(L"NOM_PRE");
Symbol CASymbolicConstants::EVENT_SYM = Symbol(L"Event");

Symbol CASymbolicConstants::EXPLICIT_SYM = Symbol(L"EXPLICIT");
Symbol CASymbolicConstants::NONE_SYM = Symbol(L"NONE");
Symbol CASymbolicConstants::GEN_SYM = Symbol(L"GEN");
Symbol CASymbolicConstants::SPC_SYM = Symbol(L"SPC");

Symbol CASymbolicConstants::VDR_GEN_SPECIFIC = Symbol(L"Specific");
Symbol CASymbolicConstants::VDR_GEN_GENERIC = Symbol(L"Generic");
Symbol CASymbolicConstants::VDR_POL_POSITIVE = Symbol(L"Positive");
Symbol CASymbolicConstants::VDR_POL_NEGATIVE = Symbol(L"Negative");
Symbol CASymbolicConstants::VDR_TENSE_UNSPECIFIED = Symbol(L"Unspecified");
Symbol CASymbolicConstants::VDR_TENSE_PAST = Symbol(L"Past");
Symbol CASymbolicConstants::VDR_TENSE_PRESENT = Symbol(L"Present");
Symbol CASymbolicConstants::VDR_TENSE_FUTURE = Symbol(L"Future");
Symbol CASymbolicConstants::VDR_MOD_ASSERTED = Symbol(L"Asserted");
Symbol CASymbolicConstants::VDR_MOD_OTHER = Symbol(L"Other");

Symbol CASymbolicConstants::RDR_TENSE_UNSPECIFIED = Symbol(L"Unspecified");
Symbol CASymbolicConstants::RDR_TENSE_PAST = Symbol(L"Past");
Symbol CASymbolicConstants::RDR_TENSE_PRESENT = Symbol(L"Present");
Symbol CASymbolicConstants::RDR_TENSE_FUTURE = Symbol(L"Future");
Symbol CASymbolicConstants::RDR_MOD_ASSERTED = Symbol(L"Asserted");
Symbol CASymbolicConstants::RDR_MOD_OTHER = Symbol(L"Other");
