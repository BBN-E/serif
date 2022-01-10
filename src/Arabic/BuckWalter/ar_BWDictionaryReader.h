// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_BWDICTIONARYREADER_H
#define AR_BWDICTIONARYREADER_H

#include "Generic/theories/Lexicon.h"
#include "Generic/common/UnexpectedInputException.h"

/** This class is never instantiated -- it's just a place to
  * put functions. */
class BWDictionaryReader {

public:
	static Lexicon* readBWDictionaryFile(const char *bw_dict_file);
	static void addToBWDictionary(Lexicon* lex, const char *bw_dict_file);
};

#endif
