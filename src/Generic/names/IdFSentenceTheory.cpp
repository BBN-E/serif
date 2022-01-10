// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/names/IdFSentenceTheory.h"
#include "Generic/names/IdFSentenceTokens.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/names/NameClassTags.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UnexpectedInputException.h"

size_t IdFSentenceTheory::THEORIES_PER_ALLOCATION=100;
IdFSentenceTheory::TheoryPool IdFSentenceTheory::theoryPool(THEORIES_PER_ALLOCATION);

//
// CONSTRUCTORS
//

/** initialize IdFSentenceTheory */
IdFSentenceTheory::IdFSentenceTheory() 
	: _length(0), _beta_score(0), 
	  _best_possible_score(0), MAX_LENGTH(MAX_SENTENCE_TOKENS)
{}

/** initialize IdFSentenceTheory with a certain length */
IdFSentenceTheory::IdFSentenceTheory(int length, int default_tag) 
	: _beta_score(0), _best_possible_score(0), MAX_LENGTH(MAX_SENTENCE_TOKENS)
{
	if (length > MAX_LENGTH)
		length = MAX_LENGTH;
	_length = length;
	for (int i = 0; i < _length; i++) {
		_tagArray[i] = default_tag;
	}
}

/** initialize IdFSentenceTheory from another IdFSentenceTheory */
IdFSentenceTheory::IdFSentenceTheory(IdFSentenceTheory *theory)
	: MAX_LENGTH(MAX_SENTENCE_TOKENS)
{
	_length = theory->getLength();
	for (int i = 0; i < _length; i++) {
		_tagArray[i] = theory->getTag(i);
	}
	_beta_score = theory->getBetaScore();
	_best_possible_score = theory->getBestPossibleScore();

}

//
// TAG ACCESS FUNCTIONS
//

/** returns the integer value of the tag at this position (used to interact
with the NameClassTags object) */
int IdFSentenceTheory::getTag(int index) {
	if (index >= _length) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTheory::getTag()", _length, index);
	}
	return _tagArray[index];
}

/** set tag at given index */
void IdFSentenceTheory::setTag(int index, int tag) { 
	if (index >= MAX_LENGTH) {
		throw InternalInconsistencyException::arrayIndexException(
			"IdFSentenceTheory::getTag()", MAX_LENGTH, index);
	}
	_tagArray[index] = tag; 
}

//
// LENGTH ACCESS FUNCTIONS
//

/** get length of sentence */
int IdFSentenceTheory::getLength() { return _length; }
/** set length of sentence */
void IdFSentenceTheory::setLength(int length) { 
	// add error message?
	if (length > MAX_LENGTH)
		length = MAX_LENGTH;
	_length = length;
}

//
// SCORE ACCESS FUNCTIONS
//

double IdFSentenceTheory::getBetaScore() { return _beta_score; }
void IdFSentenceTheory::incrementBetaScore(double new_score) { _beta_score += new_score; }
double IdFSentenceTheory::getBestPossibleScore() { return _best_possible_score; }
void IdFSentenceTheory::setBestPossibleScore(double new_score) { _best_possible_score = new_score; }

