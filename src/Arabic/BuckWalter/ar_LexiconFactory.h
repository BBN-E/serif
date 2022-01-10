// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_LEXICON_FACTORY_H
#define AR_LEXICON_FACTORY_H

#include "Generic/theories/Lexicon.h"
#include "Generic/common/SessionLogger.h"
#include "Arabic/BuckWalter/ar_BWDictionaryReader.h"
#include <string>

struct ArabicLexiconFactory: public Lexicon::Factory {
	Lexicon* build() {
		if (ParamReader::isParamTrue("ignore_lexical_dictionary")) {
			return _new Lexicon(static_cast<size_t>(0));
		}
		else {
			//read lexical dictionary
			std::string buffer = ParamReader::getRequiredParam("BWMorphDict");
			Lexicon* lex = BWDictionaryReader::readBWDictionaryFile(buffer.c_str());

			SessionLogger::logger->reportInfoMessage() <<"Read BW Lexical Dictionary: "
									<<buffer<<" "<<(int)lex->getNEntries()	<<" entries\n";
			return lex;
		}
	}
};

#endif
