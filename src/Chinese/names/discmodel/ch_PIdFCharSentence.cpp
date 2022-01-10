// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/OutputUtil.h"
#include "Chinese/tokens/ch_Tokenizer.h"
#include "Chinese/tokens/ch_Untokenizer.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Chinese/names/discmodel/ch_PIdFCharSentence.h"

using namespace std;

const Symbol PIdFCharSentence::NONE_ST = Symbol(L"NONE-ST");
const Symbol PIdFCharSentence::NONE_CO = Symbol(L"NONE-CO");


PIdFCharSentence::PIdFCharSentence(const DTTagSet *tagSet,
						   const TokenSequence &tokenSequence)
	: _tagSet(tagSet), marginScore(0),
	  _chars(0), _char_token_map(0), _tags(0),
	  _tokenSequence(&tokenSequence)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);

	_max_length = 0;
	for (int k = 0; k < tokenSequence.getNTokens(); k++) {
		Symbol pToken = tokenSequence.getToken(k)->getSymbol();
		_max_length += static_cast<int>(wcslen(pToken.to_string()));
	}

	_chars = _new Symbol[_max_length];
	_char_token_map = _new int[_max_length];
	_tags = _new int[_max_length];

	_length = 0;
	_char_buffer[1] = L'\0';

	for (int i = 0; i < tokenSequence.getNTokens(); i++) {
		Symbol pToken = tokenSequence.getToken(i)->getSymbol();
		ChineseUntokenizer::untokenize(pToken.to_string(), _word_buffer, MAX_TOKEN_SIZE+1);
		int j = 0;
		while (_word_buffer[j] != L'\0') {
			// set buffer to the character
			_char_buffer[0] = _word_buffer[j];
			_chars[_length] = ChineseTokenizer::getSubstitutionSymbol(Symbol(_char_buffer));
			_tags[_length] = -1;
			// record which token the character comes from
			_char_token_map[_length] = i;
			_length++;
			j++;
		}
	}
}


PIdFCharSentence::PIdFCharSentence(const DTTagSet *tagSet, int max_length)
	: _tagSet(tagSet), _max_length(max_length), marginScore(0),
	  _chars(0), _char_token_map(0), _tags(0), _tokenSequence(0)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);

	_chars = _new Symbol[_max_length];
	_char_token_map = _new int[_max_length];
	_tags = _new int[_max_length];
}


void PIdFCharSentence::populate(const PIdFCharSentence &other) {
	_tokenSequence = other._tokenSequence;
	_max_length = other._length;
	_tagSet = other._tagSet;
	_NONE_ST_index = other._NONE_ST_index;
	_NONE_CO_index = other._NONE_CO_index;
	marginScore = other.marginScore;

	delete[] _chars;
	delete[] _char_token_map;
	delete[] _tags;
	_chars = _new Symbol[_max_length];
	_char_token_map = _new int[_max_length];
	_tags = _new int[_max_length];

	_length = other._length;
	for (int i = 0; i < _length; i++) {
		_chars[i] = other._chars[i];
		_char_token_map[i] = other._char_token_map[i];
		_tags[i] = other._tags[i];
	}
}


Symbol PIdFCharSentence::getChar(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _chars[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFCharSentence::getChar()", _length, i);
	}
}

int PIdFCharSentence::getTokenFromChar(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _char_token_map[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFCharSentence::getTokenFromChar()", _length, i);
	}
}

int PIdFCharSentence::getTag(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _tags[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFCharSentence::getTag()", _length, i);
	}
}

void PIdFCharSentence::setTag(int i, int tag) {
	if ((unsigned) i < (unsigned) _length) {
		_tags[i] = tag;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFCharSentence::setTag()", _length, i);
	}
}

void PIdFCharSentence::dump(std::ostream &out, int indent) const {
	char *newline = OutputUtil::getNewIndentedLinebreakString(indent);

	out << "PIdFCharSentence (" << _length << " chars):";

	if (_length > 0) {
		out << newline << "  ";
		for (int i = 0; i < _length; i++) {
			out << _chars[i].to_debug_string() << " ";
		}
	}

	delete[] newline;
}
