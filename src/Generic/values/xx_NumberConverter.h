// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef XX_NUMBER_CONVERTER_H
#define XX_NUMBER_CONVERTER_H

#include "Generic/values/NumberConverter.h"
#include <limits.h>

class GenericNumberConverter : public NumberConverter {
private:
	friend class GenericNumberConverterFactory;

protected:

	virtual int convertComplexTwoPartNumberWord(const std::wstring& str) { return INT_MIN; }
	virtual int convertSpelledOutNumberWord(const std::wstring& input_str) { return INT_MIN; }
	virtual bool isSpelledOutNumberWord(const std::wstring& input_str) { return false; }

private:
	GenericNumberConverter() {}
};

class GenericNumberConverterFactory: public NumberConverter::Factory {
	virtual NumberConverter *build() { return _new GenericNumberConverter(); }
};

#endif
