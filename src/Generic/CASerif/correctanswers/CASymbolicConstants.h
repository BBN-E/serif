// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CA_WORD_CONSTANTS_H
#define CA_WORD_CONSTANTS_H

#include "common/Symbol.h"

class CASymbolicConstants
{

public:
    static Symbol NOM;
    static Symbol PRO;
    
	static Symbol FALSE_SYM;
	static Symbol TRUE_SYM;
	static Symbol NOMINAL_LOWER;
	static Symbol NOMINAL_UPPER;
	static Symbol NOMINAL_ERE;
	static Symbol NAME_LOWER;
	static Symbol NAME_UPPER;
    static Symbol NAME_ERE;
	static Symbol DATETIME_LOWER;
	static Symbol DATETIME_UPPER;
	static Symbol PRONOUN_LOWER;
	static Symbol PRONOUN_UPPER;	
	static Symbol PRONOUN_ERE;	
	static Symbol PRE_UPPER;
	static Symbol NOM_PRE_UPPER;
	static Symbol EVENT_SYM;
	static Symbol EXPLICIT_SYM;
	static Symbol NONE_SYM;
	static Symbol NONE_ERE;
	static Symbol GEN_SYM;
	static Symbol SPC_SYM;

	static Symbol VDR_GEN_SPECIFIC;
	static Symbol VDR_GEN_GENERIC;
	static Symbol VDR_POL_POSITIVE;
	static Symbol VDR_POL_NEGATIVE;
	static Symbol VDR_TENSE_UNSPECIFIED;
	static Symbol VDR_TENSE_PAST;
	static Symbol VDR_TENSE_PRESENT;
	static Symbol VDR_TENSE_FUTURE;
	static Symbol VDR_MOD_ASSERTED;
	static Symbol VDR_MOD_OTHER;
	
	static Symbol RDR_TENSE_UNSPECIFIED;
	static Symbol RDR_TENSE_PAST;
	static Symbol RDR_TENSE_PRESENT;
	static Symbol RDR_TENSE_FUTURE;
	static Symbol RDR_MOD_ASSERTED;
	static Symbol RDR_MOD_OTHER;
};



#endif
