// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_SENTENCE_H
#define IDF_SENTENCE_H

#include "Generic/common/limits.h"
#include "Generic/common/Symbol.h"
#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/IdFSentenceTokens.h"
#include <wchar.h>

class UTF8InputStream;
class UnexpectedInputException;
class IdFListSet;
#include "Generic/names/NameClassTags.h"


/**
 * The combination of the tokens and the name mark-up of a sentence. Used in training.
 */
class IdFSentence {
protected:
	int _length;
	int _maxLength;
	IdFSentenceTokens *_tokens;
	IdFSentenceTheory *_theory;
	const NameClassTags *_nameClassTags;
	const IdFListSet *_listSet;

public:
	IdFSentence(const NameClassTags *nameClassTags, const IdFListSet *listSet = 0);

	IdFSentence &operator=(const IdFSentence &other);


	bool readTrainingSentence(UTF8InputStream& stream) throw(UnexpectedInputException);

	Symbol getFullTag(int index) { 
		return _nameClassTags->getTagSymbol(_theory->getTag(index)); 
	}
	Symbol getReducedTag(int index) { 
		return _nameClassTags->getReducedTagSymbol(_theory->getTag(index)); 
	}
	Symbol getTagStatus(int index) { 
		return _nameClassTags->getTagStatus(_theory->getTag(index)); 
	}
	int getTag(int index) { 
		return _theory->getTag(index); 
	}
	void setTag(int index, int tag) { _theory->setTag(index, tag); }
	
	// see documention in IdFSentenceTokens
	Symbol getWord(int index) { return _tokens->getWord(index); }
	void setWord(int index, Symbol word) { _tokens->setWord(index, word); }

	int getLength() { return _length; }
	void setLength(int length) { 
		_length = length;
		_theory->setLength(_length);
		_tokens->setLength(_length);
	}


	void setTheory(IdFSentenceTheory * theory) {
		_theory=theory;
	}

	void setTokens(IdFSentenceTokens * tokens) {
		_tokens=tokens;
	}
	
	// debugging output functions
	std::wstring to_just_tokens_string();
	std::wstring to_string();
	std::wstring to_sgml_string();
	std::wstring to_enamex_sgml_string();

};


#endif
