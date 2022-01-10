// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AR_WORD_SEGMENT_H
#define AR_WORD_SEGMENT_H
#include "Arabic/parse/ar_Segment.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SessionLogger.h"
#define MAX_SEG_PER_WORD 4		//Conj, Prep/Part, Root, PronounSuffix
#define CHART_TOKEN_MAX_POSS 20
class WordAnalysis{
public:
struct {
		Segment* segments[MAX_SEG_PER_WORD];
		int numSeg;
	};
};

class WordSegment {

private:
	static const size_t max_possibilities;
	static const size_t max_segments;
	static const int CONJ =3;
	static const int PREP = 2;
	static const int PART = 1;
	static const int NONE = 0;

	bool _emptyLeaf;

	Symbol _originalWord;
    WordAnalysis _possibilities[CHART_TOKEN_MAX_POSS];
	int _num_possibilities;
	wchar_t _str_buff[50];

	void _addPossibility(const Symbol* s, const int* start, const int* type, int numStr);
	void _addComplexPossibility(const Symbol* s, const int* places,
								 const int* type, int suffix, int numStr);
	void _fixSegmentation();

	Symbol _subWord(const wchar_t* str, size_t start, size_t end);
	int _suffix(const wchar_t* word);
	int _prefix(const wchar_t* word, int start);

public:
	struct SEGMENTATION{
		Symbol segments[10];
		int numSeg;
	};
	typedef SEGMENTATION Segmentation;

	WordSegment() : _num_possibilities(0), _emptyLeaf(false) {}
	~WordSegment();
	WordSegment(const WordSegment::Segmentation* possibilities, int num_poss);

	void getSegments(Symbol token);
	int maxLength() const;
	int lengthOf(int index) const;
	int getNumPossibilities() const;
	WordAnalysis getPossibility(int index) const;
	WordAnalysis getLongest() const;
	int getMaxSegments(const wchar_t *word);
	Symbol getOriginalWord(){return _originalWord;} ;
	void printSegment();





};
#endif
