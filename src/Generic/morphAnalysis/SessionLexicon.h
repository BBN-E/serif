// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SESSION_LEXICON_H
#define SESSION_LEXICON_H

#include "Generic/theories/Lexicon.h"

// For backwards compatibility:
class SessionLexicon {
public:
	static SessionLexicon getInstance() { return SessionLexicon(); }
	Lexicon* getLexicon() {
		return Lexicon::getSessionLexicon();
	}
	static void destroy() {
		Lexicon::destroySessionLexicon();
	}
	static LexicalEntry* getLexicalEntryByID(std::size_t id) {
		return Lexicon::getSessionLexicon()->getEntryByID(id);
	}
};

#endif



