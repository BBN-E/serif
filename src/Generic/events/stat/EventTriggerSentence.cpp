// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/UTF8InputStream.h"
#include "common/UTF8OutputStream.h"
#include "common/UTF8Token.h"
#include "common/InternalInconsistencyException.h"
#include "common/SymbolConstants.h"
#include "theories/TokenSequence.h"
#include "discTagger/DTTagSet.h"
#include "events/stat/EventTriggerSentence.h"

using namespace std;

Token* EventTriggerSentence::_tokenBuffer[MAX_SENTENCE_TOKENS+1];

EventTriggerSentence::EventTriggerSentence(const DTTagSet *tagSet)
	: _tagSet(tagSet)
{
	_tags = _new int[MAX_SENTENCE_TOKENS];
}

TokenSequence *EventTriggerSentence::getTokenSequence() {
	return _tseq;
}

int EventTriggerSentence::getTag(int i) {
	if ((unsigned) i < (unsigned) _length) {
		return _tags[i];
	}
	else {
		throw InternalInconsistencyException::arrayIndexException(
			"EventTriggerSentence::getTag()", _length, i);
	}
}
	
bool EventTriggerSentence::readTrainingSentence(int sentno, UTF8InputStream &in) {
	// code modified from pIdFSentence.cpp

	UTF8Token token;
	if (in.eof())
		return false;
	in >> token;
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		throw UnexpectedInputException("EventTriggerSentence::readTrainingSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (token.symValue() != SymbolConstants::leftParen) {
			throw UnexpectedInputException("EventTriggerSentence::readTrainingSentence", 
				"ill-formed sentence");
		}

		in >> token;
		if (_length < MAX_SENTENCE_TOKENS) {
			_tokenBuffer[_length] = _new Token(_length, _length, token.symValue());
		}

		in >> token;
		if (_length < MAX_SENTENCE_TOKENS) {
			int tag_index = _tagSet->getTagIndex(token.symValue());
			if (tag_index < 0) {
				std::string s = "(";
				for (int i = 0; i < _length; i++) {
					s += "(";
					s += _tokenBuffer[i]->getSymbol().to_debug_string();
					s += " ";
					s += _tagSet->getTagSymbol(_tags[i]).to_debug_string();
					s += ") ";
				}
				s += "(";
				s += _tokenBuffer[_length]->getSymbol().to_debug_string();
				s += " ";
				char buffer[1001];
				_snprintf(buffer, 1000, "Unknown tag found in training file: %s\nbeginning of bogus sentence: %s", 
					token.symValue().to_debug_string(), s.c_str());
				throw UnexpectedInputException(
					"EventTriggerSentence::readTrainingSentence", buffer);
			}
			_tags[_length] = tag_index;
		}
		_length++;

		in >> token;
		if (token.symValue() != SymbolConstants::rightParen) {
			throw UnexpectedInputException(
				"EventTriggerSentence::readTrainingSentence",
				"ill-formed sentence");
		}
	}

	if (_length >= MAX_SENTENCE_TOKENS) {
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = MAX_SENTENCE_TOKENS;
	}

	_tseq = _new TokenSequence(sentno, _length, _tokenBuffer);
	return true;
}

bool EventTriggerSentence::readDecodeSentence(int sentno, UTF8InputStream &in) {
	UTF8Token token;
	if (in.eof())
		return false;
	in >> token; // (
	if (in.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen) {
		SessionLogger::info("SERIF") << token.symValue().to_debug_string() << "\n";
		throw UnexpectedInputException("EventTriggerSentence::readDecodeSentence", 
			"ill-formed sentence");
	}

	_length = 0;
	while (true) {
		in >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;
		if (in.eof())
			break;

		if (_length < MAX_SENTENCE_TOKENS) {
			_tokenBuffer[_length] = _new Token(_length, _length, token.symValue());
			_tags[_length] = -1;
		}
		_length++;
	}

	if (_length >= MAX_SENTENCE_TOKENS) {
		SessionLogger::info("SERIF") << "truncating sentence with " << _length << " tokens\n";
		_length = MAX_SENTENCE_TOKENS;
	}

	_tseq = _new TokenSequence(sentno, _length, _tokenBuffer);
	return true;
}
