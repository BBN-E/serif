// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/discTagger/DTTagSet.h"
#include "Generic/PNPChunking/PNPChunkSentence.h"
#include "Generic/theories/TokenSequence.h"

using namespace std;

const Symbol PNPChunkSentence::NONE_ST = Symbol(L"NONE-ST");
const Symbol PNPChunkSentence::NONE_CO = Symbol(L"NONE-CO");


PNPChunkSentence::PNPChunkSentence(const DTTagSet *tagSet, const int max_length)
	: _tagSet(tagSet), _max_length(max_length)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);
}
PNPChunkSentence::PNPChunkSentence(const DTTagSet *tagSet, const int max_length, 
								   const TokenSequence* tokens, const Symbol *pos)	
								   : _tagSet(tagSet), _max_length(max_length)
{
	_NONE_ST_index = _tagSet->getTagIndex(NONE_ST);
	_NONE_CO_index = _tagSet->getTagIndex(NONE_CO);
	for(int i=0; i<tokens->getNTokens(); i++){

		_words[i] = tokens->getToken(i)->getSymbol();
		_pos[i] = pos[i];
		_length = tokens->getNTokens();
	}
}

Symbol PNPChunkSentence::getWord(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _words[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PNPChunkSentence::getWord()", _length, i);
	}
}

Symbol PNPChunkSentence::getPOS(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _pos[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PNPChunkSentence::getPOS()", _length, i);
	}
}
int PNPChunkSentence::getTag(int i) const {
	if ((unsigned) i < (unsigned) _length) {
		return _tags[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PNPChunkSentence::getTag()", _length, i);
	}
}


void PNPChunkSentence::addWord(Symbol word) {
	if (_length < _max_length) {
		_words[_length] = word;
		_tags[_length] = -1;
		_length++;
	}
}

void PNPChunkSentence::setTag(int i, int tag) {
	if ((unsigned) i < (unsigned) _length) {
		_tags[i] = tag;
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"PNPChunkSentence::setTag()", _length, i);
	}
}

	
bool PNPChunkSentence::readTrainingSentence(UTF8InputStream &in) {
	// code largely copied from IdFSentence.cpp

	UTF8Token token;
	if (in.eof())
		return false;
	in >> token;
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
			throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
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
					"PNPChunkSentence::readTrainingSentence", buffer);
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
				"PNPChunkSentence::readTrainingSentence",
				"ill-formed sentence");
		}
	}

	if (_length >= _max_length) {
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}
bool PNPChunkSentence::readPOSTrainingSentence(UTF8InputStream &in){
	// code largely copied from IdFSentence.cpp

	UTF8Token token;
	if (in.eof())
		return false;
	in >> token;
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
			throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
				"ill-formed sentence");
		}

		in >> token;
		if (_length < _max_length) {
			_words[_length] = token.symValue();
		}
		in >> token;
		if (_length < _max_length) {
			_pos[_length] = token.symValue();
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
					"PNPChunkSentence::readTrainingSentence", buffer);
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
				"PNPChunkSentence::readTrainingSentence",
				"ill-formed sentence");
		}
	}

	if (_length >= _max_length) {
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}

bool PNPChunkSentence::readPOSTestSentence(UTF8InputStream &in){
	// code largely copied from IdFSentence.cpp

	UTF8Token token;
	if (in.eof())
		return false;
	in >> token;
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
			throw UnexpectedInputException("PNPChunkSentence::readTrainingSentence", 
				"ill-formed sentence");
		}

		in >> token;
		if (_length < _max_length) {
			_words[_length] = token.symValue();
		}
		in >> token;
		if (_length < _max_length) {
			_pos[_length] = token.symValue();
		}		


		if (_length < _max_length) {
			_tags[_length] = -1;
		}
		_length++;
		in >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			throw UnexpectedInputException(
				"PNPChunkSentence::readTrainingSentence",
				"ill-formed sentence");
		}
	}

	if (_length >= _max_length) {
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}
bool PNPChunkSentence::readSexpSentence(UTF8InputStream &in) {
	UTF8Token token;
	if (in.eof())
		return false;
	in >> token; // (
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("PNPChunkSentence::readSexpSentence", 
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
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = _max_length;
	}

	return true;
}

void PNPChunkSentence::writeSexp(UTF8OutputStream &out) {
	out << L"(";
	for (int i = 0; i < _length; i++) {
		if (i > 0)
			out << L" ";
		out << L"(" << _words[i].to_string() << L" "
			<< _tagSet->getTagSymbol(_tags[i]).to_string() << L")";
	}
	out << L")\n";
}

