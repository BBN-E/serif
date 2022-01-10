// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"

/** initialize IdFSentenceTokens */
IdFSentenceTokens::IdFSentenceTokens() : MAX_LENGTH(MAX_SENTENCE_TOKENS), _length(0) {
	for (int i = 0; i < MAX_LENGTH; i++) {
		_constraints[i] = false;
	}
}

/**
 * Fills in _wordArray for next sentence in stream (not used in SERIF)
 * 
 * @return true if successful in reading new sentence, false if EOF or other error
 */
bool IdFSentenceTokens::readDecodeSentence(UTF8InputStream& stream) {

	// expected sample sentence:
	// (the United Nations succeeded .)

	UTF8Token token;
	if (stream.eof())
		return false;
	stream >> token;
	if (stream.eof())
		return false;

	if (token.symValue() != SymbolConstants::leftParen)
		throw UnexpectedInputException("IdFSentenceTokens::readDecodeSentence", 
		"ill-formed sentence");

	_length = 0;
	while (true) {
		stream >> token;
		if (token.symValue() == SymbolConstants::rightParen)
			break;

		if (_length < MAX_LENGTH) {
			_wordArray[_length] = token.symValue();
			_constraints[_length] = false;
		}
		_length++;
	}
	
	if (_length >= MAX_LENGTH) {
		std::cerr << "truncating sentence with " << _length << " tokens\n";
		_length = MAX_LENGTH-1;
	}

	return true;


}

bool IdFSentenceTokens::isConstrained(int index) {
	if (index >= _length) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTokens::getWord()", _length, index);
	}
	return _constraints[index];

}

void IdFSentenceTokens::constrainToBreakBefore(int index, bool constrain) {
	if (index >= MAX_LENGTH) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTokens::getWord()", MAX_LENGTH, index);
	}
	_constraints[index] = constrain;
}


/** get word at given index */
Symbol IdFSentenceTokens::getWord(int index) {
	if (index >= _length) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTokens::getWord()", _length, index);
	}
	return _wordArray[index];
}

/** set word at given index */
void IdFSentenceTokens::setWord(int index, Symbol word) {
	if (index >= MAX_LENGTH) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTokens::setWord()", MAX_LENGTH, index);
	}
	_wordArray[index] = word;
}

/** get length */
int IdFSentenceTokens::getLength() { return _length; }
/** set length */
void IdFSentenceTokens::setLength(int length) { 
	if (length > MAX_LENGTH)
		length = MAX_LENGTH;
	_length = length;
}

/**
 * prints out: word word word
 */
std::wstring IdFSentenceTokens::to_string()
{
	std::wstring result = L"";
	for (int i = 0; i < _length; i++) {
		if (i != 0)	
			result += L" ";
		result += getWord(i).to_string();					
	}
	return result;
}
