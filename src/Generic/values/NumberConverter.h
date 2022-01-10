// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUMBER_CONVERTER_H
#define NUMBER_CONVERTER_H

#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class NumberConverter {
public:
	/** Create and return a new NumberConverter. */
	static NumberConverter *build() { return _factory()->build(); }
	/** Hook for registering new NumberConverter factories */
	struct Factory { virtual NumberConverter *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~NumberConverter() {}

	int convertNumberWord(const std::wstring& input_str);

protected:
	NumberConverter() {}

	virtual int convertComplexTwoPartNumberWord(const std::wstring& str) = 0;
	virtual int convertSpelledOutNumberWord(const std::wstring& input_str) = 0;
	virtual bool isSpelledOutNumberWord(const std::wstring& input_str) = 0;

private:
	static boost::shared_ptr<Factory> &_factory();

	int convertComplexNumberWord(const std::wstring& str);
	int calculateValue(const std::vector<int>& values);
	std::list<int> normalizeHundreds(const std::list<int>& input);
};

#endif
