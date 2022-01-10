// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/morphAnalysis/SessionLexicon.h"
/*
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/Lexicon.h"
#include "Generic/theories/LexicalEntry.h"

GenericSessionLexicon* GenericSessionLexicon::instance = 0;
bool GenericSessionLexicon::is_initialized = false;

GenericSessionLexicon::GenericSessionLexicon() : lex(0), ignore_lexicon(false) {}

GenericSessionLexicon::~GenericSessionLexicon() { delete lex; }

void GenericSessionLexicon::destroy() {
	delete instance;
	instance = 0;
	is_initialized = false;
}

Lexicon* GenericSessionLexicon::getLexicon() { 
	return lex;
}

LexicalEntry* GenericSessionLexicon::getLexicalEntryByID(size_t id) {

	if (ignore_lexicon) {
		return 0;
	}
	if (lex == 0){
		throw InternalInconsistencyException("SessionLexicon::getLexicalEntryByID()",
									"getLexicalEntry called on NULL lexicon!");
	}
	else {
		return lex->getEntryByID(id);
	}
};
*/
