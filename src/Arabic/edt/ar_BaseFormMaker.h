// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_BASE_FORM_MAKER_H
#define ar_BASE_FORM_MAKER_H

#include "Generic/common/Symbol.h"
#include "Arabic/common/ar_ArabicSymbol.h"
#include <cstddef>
using namespace std;
#define MAX_WORD_BUFFER_SZ 200
class BaseFormMaker{
private:
	//remove prefixes lAl (To the) becomes ll, and only the first l is a clitic
	static Symbol _Al;
	static Symbol _l;
	static Symbol _b;
	static Symbol _w;


	static Symbol _stAn;
	static Symbol _endings[13];
	const static int _numEndings = 13;
	static wchar_t _wordBuffer[MAX_WORD_BUFFER_SZ];
	static bool startsWith(const wchar_t* word, Symbol start, size_t index);
	static bool endsWith(const wchar_t* word, Symbol end, size_t index);


public:
	static Symbol getGPEBaseForm(Symbol word);
	static Symbol removeNameClitics(Symbol word);
	static Symbol getPERBaseForm(Symbol word);

};
#endif
