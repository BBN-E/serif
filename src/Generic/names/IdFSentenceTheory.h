// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef IDF_SENTENCE_THEORY_H
#define IDF_SENTENCE_THEORY_H

#include "Generic/common/limits.h"
#include "Generic/common/MemoryPool.h"
#include <wchar.h>

class NameClassTags;
class Symbol;
class UnexpectedInputException;

/** 
 * The name mark-up for a sentence. Tags are stored as integers. Also contains scores 
 * for use in n-best decoding. At the end of n-best decoding, _best_possible_score 
 * contains the score for the whole theory.
 */
class IdFSentenceTheory {
private:

	int _length;
	int _tagArray[MAX_SENTENCE_TOKENS];

	// used only in n-best decoding
	double _beta_score;
	double _best_possible_score;

public:
	IdFSentenceTheory();
	IdFSentenceTheory(int length, int default_tag);
	IdFSentenceTheory(IdFSentenceTheory *theory);
	
	const int MAX_LENGTH;
	int getTag(int index);
	void setTag(int index, int tag);

	int getLength();
	void setLength(int length);
	
	double getBetaScore();
	void incrementBetaScore(double new_score);

	double getBestPossibleScore();
	void setBestPossibleScore(double new_score);

	static void* operator new(size_t size) {
		return theoryPool.allocate(size);
	}
	static void operator delete(void* object, size_t size) {
		theoryPool.deallocate(static_cast<IdFSentenceTheory*>(object), size);
	}
	
	
    static size_t THEORIES_PER_ALLOCATION; //defaults to 100 as set in IdFSentenceTheory.cpp

	static void setTheoryCount (size_t num_theories) {
		theoryPool.setItemsPerBlock(num_theories);
		THEORIES_PER_ALLOCATION=num_theories;
	}

	size_t getTheoryCount() {
		return THEORIES_PER_ALLOCATION;
	}

private:
	typedef MemoryPool<IdFSentenceTheory> TheoryPool;
	static TheoryPool theoryPool;

};


#endif
