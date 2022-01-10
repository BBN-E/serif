// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ENGLISH_NUMBER_CONVERTER_H
#define ENGLISH_NUMBER_CONVERTER_H

#include "Generic/values/NumberConverter.h"

class EnglishNumberConverter : public NumberConverter {
private:
	friend class EnglishNumberConverterFactory;

protected:

	virtual int convertSpelledOutNumberWord(const std::wstring& input_str);
	virtual int convertComplexTwoPartNumberWord(const std::wstring& str);
	virtual bool isSpelledOutNumberWord(const std::wstring& input_str);

private:
	EnglishNumberConverter() {}
};

class EnglishNumberConverterFactory: public NumberConverter::Factory {
	virtual NumberConverter *build() { return _new EnglishNumberConverter(); }
};

#endif
