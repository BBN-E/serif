// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/names/discmodel/PIdFSentence.h"

using namespace std;

const Symbol PIdFSentence::NONE_ST = Symbol(L"NONE-ST");
const Symbol PIdFSentence::NONE_CO = Symbol(L"NONE-CO");


PIdFSentence::PIdFSentence(const DTTagSet *tagSet,
						   const TokenSequence &tokenSequence,
						   bool force_tokens_lowercase)
	: _tagSet(tagSet), _max_length(tokenSequence.getNTokens()), 
	  _in_training_set(false), marginScore(0), _tokenSequence(&tokenSequence)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);

	_words = _new Symbol[_max_length];
	_tags = _new int[_max_length];

	_length = tokenSequence.getNTokens();
	for (int i = 0; i < _length; i++) {
		if (force_tokens_lowercase) 
			_words[i] = SymbolUtilities::lowercaseSymbol(tokenSequence.getToken(i)->getSymbol());
		else _words[i] = tokenSequence.getToken(i)->getSymbol();
		_tags[i] = -1;
	}
}


PIdFSentence::PIdFSentence(const DTTagSet *tagSet, int max_length)
	: _tagSet(tagSet), _max_length(max_length), _length(0), 
	  _in_training_set(false), marginScore(0), _tokenSequence(0)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);

	_words = _new Symbol[_max_length];
	_tags = _new int[_max_length];
}


void PIdFSentence::populate(const PIdFSentence &other) {
	_tokenSequence = other._tokenSequence;
	_in_training_set = other._in_training_set;
	_max_length = other._length;
	_tagSet = other._tagSet;
	_NONE_ST_index = other._NONE_ST_index;
	_NONE_CO_index = other._NONE_CO_index;
	marginScore = other.marginScore;

	delete[] _words;
	delete[] _tags;
	_words = _new Symbol[_max_length];
	_tags = _new int[_max_length];

	_length = other._length;
	for (int i = 0; i < _length; i++) {
		_words[i] = other._words[i];
		_tags[i] = other._tags[i];
	}
}


Symbol PIdFSentence::getWord(int i) {
	if ((unsigned) i < (unsigned) _length) {
		return _words[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFSentence::getWord()", _length, i);
	}
}

int PIdFSentence::getTag(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _tags[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFSentence::getTag()", _length, i);
	}
}


void PIdFSentence::addWord(Symbol word) {
	if (_length < _max_length) {
		_words[_length] = word;
		_tags[_length] = -1;
		_length++;
	}
}

void PIdFSentence::setTag(int i, int tag) {
	if ((unsigned) i < (unsigned) _length) {
		_tags[i] = tag;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PIdFSentence::setTag()", _length, i);
	}
}

void PIdFSentence::changeToStartTag(int i) {
	int tag = _tags[i];
	if (_tagSet->isSTTag(tag)) 
		return;

	Symbol reducedTag = _tagSet->getReducedTagSymbol(tag);
	wstring reducedTagStr = reducedTag.to_string();
	reducedTagStr = reducedTagStr.append(L"-ST");

	_tags[i] = _tagSet->getTagIndex(Symbol(reducedTagStr));
}

void PIdFSentence::changeToContinueTag(int i) {
	int tag = _tags[i];
	if (_tagSet->isCOTag(tag)) 
		return;

	Symbol reducedTag = _tagSet->getReducedTagSymbol(tag);
	wstring reducedTagStr = reducedTag.to_string();
	reducedTagStr = reducedTagStr.append(L"-CO");

	_tags[i] = _tagSet->getTagIndex(Symbol(reducedTagStr));
}
	
bool PIdFSentence::readTrainingSentence(UTF8InputStream &in) {
	// code largely copied from IdFSentence.cpp

	UTF8Token token;
	if (in.eof())
		return false;
	in >> token;
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PIdFSentence::readTrainingSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
			throw UnexpectedInputException("PIdFSentence::readTrainingSentence", 
				"ill-formed sentence");
		}

		in >> token;
		if (_length < _max_length) {
			_words[_length] = token.symValue();
		}

		in >> token;
		if (_length < _max_length) {
			int tag_index = _tagSet->getTagIndex(token.symValue());
			if (tag_index < 0) {
				std::string s = "(";
				for (int i = 0; i < _length; i++) {
					s += "(";
					s += _words[i].to_debug_string();
					s += " ";
					s += _tagSet->getTagSymbol(_tags[i]).to_debug_string();
					s += ") ";
				}
				s += "(";
				s += _words[_length].to_debug_string();
				s += " ";
				char buffer[1001];
				_snprintf(buffer, 1000, "Unknown tag found in training file: %s\nbeginning of bogus sentence: %s", 
					token.symValue().to_debug_string(), s.c_str());
				throw UnexpectedInputException(
					"PIdFSentence::readTrainingSentence", buffer);
			}
			_tags[_length] = tag_index;

			// if we have NONE-?? NONE-ST, change to NONE-?? NONE-CO
			if (_length != 0 &&
				_tags[_length] == _NONE_ST_index &&
				(_tags[_length-1] == _NONE_ST_index ||
				 _tags[_length-1] == _NONE_CO_index))
			{
				_tags[_length] = _NONE_CO_index;
			}
		}
		_length++;

		in >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			throw UnexpectedInputException(
				"PIdFSentence::readTrainingSentence",
				"ill-formed sentence");
		}
	}

	if (_length >= _max_length) {
		std::cerr << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}

bool PIdFSentence::readSexpSentence(UTF8InputStream &in) {
	UTF8Token token;
	if (in.eof())
		return false;
	in >> token; // (
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PIdFSentence::readSexpSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (in.eof())
			break;

		if (_length < _max_length) {
			_words[_length] = token.symValue();
			_tags[_length] = -1;
		}
		_length++;
	}

	if (_length >= _max_length) {
		std::cerr << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}

void PIdFSentence::writeSexp(UTF8OutputStream &out) {
	out << L"(";
	for (int i = 0; i < _length; i++) {
		if (i > 0)
			out << L" ";
		out << L"(" << _words[i].to_string() << L" "
			<< _tagSet->getTagSymbol(_tags[i]).to_string() << L")";
	}
	out << L")\n";
}

