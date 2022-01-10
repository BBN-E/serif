// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LEXICAL_TOKEN_SEQUENCE_H
#define LEXICAL_TOKEN_SEQUENCE_H

#include "Generic/common/Symbol.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/LexicalToken.h"
#include "Generic/theories/TokenSequence.h"

#include "Generic/theories/SentenceSubtheory.h"
#include "Generic/common/limits.h"
#include "Generic/theories/SentenceTheory.h"


class StateSaver;
class StateLoader;
class ObjectIDTable;
class ObjectPointerTable;


class LexicalTokenSequence :public TokenSequence {
public:
	LexicalTokenSequence();
	LexicalTokenSequence(int sent_no, int n_tokens, Token* tokens[]);
	virtual ~LexicalTokenSequence();

	//This class stores the original token sequence 
	//that was used to create it. Use this method to 
	//access this information.
	const LexicalToken *getOriginalToken(int i) const;
	int getNOriginalTokens() const { return _n_orig_tokens; }

	const LexicalToken *getToken(int i) const {
		return dynamic_cast<const LexicalToken*>(TokenSequence::getToken(i)); }
	const LexicalToken *getToken(size_t i) const {
		return dynamic_cast<const LexicalToken*>(TokenSequence::getToken(i)); }

	virtual void retokenize(int n_tokens, Token* tokens[]);

	// For saving state:
	virtual void updateObjectIDTable() const;
	virtual void saveState(StateSaver *stateSaver) const;
	// For loading state:
	LexicalTokenSequence(StateLoader *stateLoader);
	virtual void resolvePointers(StateLoader *stateLoader);
	// For XML serialization:
	virtual void saveXML(SerifXML::XMLTheoryElement elem, const Theory *context=0) const;
	explicit LexicalTokenSequence(SerifXML::XMLTheoryElement elem, int sent_no);

private:
	int _n_orig_tokens;
	LexicalToken **_originaltokens;
};

#endif
