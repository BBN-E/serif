// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFSentence.h"
#include "Generic/names/IdFListSet.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"

/** initialize IdFSentenceTokens */
IdFSentence::IdFSentence(const NameClassTags *nameClassTags, const IdFListSet *listSet) 
	: _maxLength(MAX_SENTENCE_TOKENS), _nameClassTags(nameClassTags),
	  _listSet(listSet)
{
	_tokens = _new IdFSentenceTokens();
	_theory = new IdFSentenceTheory();
	_length = 0;
}


IdFSentence &IdFSentence::operator=(const IdFSentence &other) {
	if (other._length <= _maxLength)
		_length = other._length;
	else
		_length = _maxLength;

	_tokens->setLength(_length);
	for (int i = 0; i < _length; i++)
		_tokens->setWord(i, other._tokens->getWord(i));

	_theory->setLength(_length);
	for (int j = 0; j < _length; j++)
		_theory->setTag(j, other._theory->getTag(j));

	_nameClassTags = other._nameClassTags;
	_listSet = other._listSet;

	return *this;
}


/** 
* Reads training sentence from filestream (i.e. fills in _tokens and _theory)
* 
* @return true if successful in reading new sentence, false if EOF or other error
*/
bool IdFSentence::readTrainingSentence(UTF8InputStream& stream) 
throw(UnexpectedInputException) {
	
	// expected sample sentence:
	// ((the NONE-ST)(United ORG-ST)(Nations ORG-CO)(succeeded NONE-ST)(. NONE-CO))

	UTF8Token token;
	if (stream.eof())
		return false;
	stream >> token;
	if (stream.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen)
		throw UnexpectedInputException("IdFSentenceTokens::readTrainingSentence", 
		"ill-formed sentence");

	int length = 0;
	while (true) {
		stream >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen)
		throw UnexpectedInputException("IdFSentenceTokens::readTrainingSentence", 
			"ill-formed sentence");

		stream >> token;
		if (length < _maxLength) {
			setLength(length + 1);
			_tokens->setWord(length, token.symValue());
		}
		stream >> token;
		if (length < _maxLength) {
			int tag_num = _nameClassTags->getIndexForTag(token.symValue());
			if (tag_num < 0) {
				std::string s = "(";
				for (int i = 0; i < length; i++) {
					s += "(";
					s += _tokens->getWord(i).to_debug_string();
					s += " ";
					s += _nameClassTags->getTagSymbol(_theory->getTag(i)).to_debug_string();
					s += ") ";
				}
				s += "(";
				s += _tokens->getWord(length).to_debug_string();
				s += " ";
				char buffer[1001];
				_snprintf(buffer, 1000, "unknown tag found in training file: %s\nbeginning of errorful sentence: %s", 
					token.symValue().to_debug_string(), s.c_str());
				throw UnexpectedInputException("IdFSentenceTokens::readTrainingSentence", 
					buffer);
			}
			_theory->setTag(length, tag_num);

			// if we have NONE-?? NONE-ST, change to NONE-?? NONE-CO
			if (length != 0 &&
				_theory->getTag(length) == _nameClassTags->getNoneStartTagIndex() &&
				(_theory->getTag(length-1) == _nameClassTags->getNoneStartTagIndex() ||
				 _theory->getTag(length-1) == _nameClassTags->getNoneContinueTagIndex()))
				_theory->setTag(length, _nameClassTags->getNoneContinueTagIndex());

			// X-ST and X=Y-STST can follow anything.
			// Y-CO and X=Y-ST/-CO must come after something with a parent of Y
			// further, X=Y-CO must come after X=Y-ST/CO
			// otherwise, warn!
			// the getparent method, becauseit defaults to getreduced for nested tags, is
			// applicable here


			bool warn = false;
			// diagnostics
			if (length != 0) {
				if (!_nameClassTags->isStart(_theory->getTag(length))) {
					if (_nameClassTags->getParentTagSymbol(_theory->getTag(length)) !=
						_nameClassTags->getParentTagSymbol(_theory->getTag(length-1)))
						warn = true;
					if (_nameClassTags->isNested(_theory->getTag(length)) && 
						_nameClassTags->isContinue(_theory->getTag(length)) && 
						(_nameClassTags->getSemiReducedTagSymbol(_theory->getTag(length)) !=
						_nameClassTags->getSemiReducedTagSymbol(_theory->getTag(length-1))))
						warn = true;
				}
			}

			if (warn)
			{
				SessionLogger::warn("idf_training_inconsistency")
					<< _nameClassTags->getTagSymbol(_theory->getTag(length-1)).to_debug_string()
					<< " followed by "
					<< _nameClassTags->getTagSymbol(_theory->getTag(length)).to_debug_string()
					<< "\n";
			}

		}
		length++;

		stream >> token;
		if (token.symValue() != SymbolConstants::rightParen)
		throw UnexpectedInputException("IdFSentenceTokens::readTrainingSentence", 
			"ill-formed sentence");
		
		
	}

	if (length >= _maxLength) {
		SessionLogger::info("SERIF") << "truncating sentence with " << length << " tokens\n";
		length = _maxLength;
	}

	setLength(length);

	if (_listSet != 0) {
		IdFSentenceTokens tempTokens;
		IdFSentenceTheory tempTheory;

		// "weird_annotation": this refers to the possibility that we have
		// a name break in the middle of what would be a list token. For
		// example, say "U.S. Army" is on our list, but our annotation
		// says <GPE>U.S.</GPE> <ORG>Army</ORG>. That's bad! So we don't
		// smush those symbols together. In decode, we might lose, but that's 
		// the way it goes when your lists are not consistent with your annotation.
		// For now, however, it does complain to the screen.
		Symbol listSym;
		int new_token_count = 0;
		for (int i = 0; i < _tokens->getLength(); ) {
			int name_length = _listSet->isListMember(_tokens, i);
			if (name_length != 0) {
				int tag = _theory->getTag(i);
				bool weird_annotation = false;
				for (int j = i + 1; j < i + name_length; j++) {
					if (_nameClassTags->isStart(_theory->getTag(j))) 
						weird_annotation = true;
				}
				if (!weird_annotation) {
					tempTokens.setWord(new_token_count, _listSet->barSymbols(_tokens, i, name_length));
					tempTheory.setTag(new_token_count, tag);
					new_token_count++;
				} else {
					SessionLogger::info("SERIF") << "name break in the middle of an item on the list: ";
					for (int j = i; j < i + name_length; j++) {
						tempTokens.setWord(new_token_count, _tokens->getWord(j));
						tempTheory.setTag(new_token_count, _theory->getTag(j));
						SessionLogger::info("SERIF") << _tokens->getWord(j).to_debug_string() << " ";
						new_token_count++;
					}
					SessionLogger::info("SERIF") << "\n";
				}
				i += name_length;
			} else {
				tempTokens.setWord(new_token_count, _tokens->getWord(i));
				tempTheory.setTag(new_token_count, _theory->getTag(i));
				i++;
				new_token_count++;
			}
		}
		tempTokens.setLength(new_token_count);
		tempTheory.setLength(new_token_count);

		for (int j = 0; j < new_token_count; j++) {
			_tokens->setWord(j, tempTokens.getWord(j));
			_theory->setTag(j, tempTheory.getTag(j));
		}
		setLength(new_token_count);
	}

	return true;


}

/**
 * prints out ((word tag)(word tag)(word tag))
 */
std::wstring IdFSentence::to_string() {
	return _nameClassTags->to_string(_tokens, _theory);
}
/**
 * prints out word <enamex type="tag">word</enamex> word
 */
std::wstring IdFSentence::to_enamex_sgml_string() {
	return _nameClassTags->to_enamex_sgml_string(_tokens, _theory);
}
/**
 * prints out: word <tag>word</tag> word
 */
std::wstring IdFSentence::to_sgml_string() {
	return _nameClassTags->to_sgml_string(_tokens, _theory);
}
/**
 * prints out: word word word
 */
std::wstring IdFSentence::to_just_tokens_string() {
	return _tokens->to_string();
}

