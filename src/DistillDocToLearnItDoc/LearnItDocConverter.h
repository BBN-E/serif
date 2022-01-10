#ifndef _LEARNIT_DOC_CONVERTER_H_
#define _LEARNIT_DOC_CONVERTER_H_

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "Generic/common/UTF8OutputStream.h"

class DocTheory;

class LearnItSentenceConverter {
public:
	LearnItSentenceConverter()  {}
	virtual void processSentence(const DocTheory* dt, int sn,
		std::wostream& out)=0;
};

typedef boost::shared_ptr<LearnItSentenceConverter> LearnItSentenceConverter_ptr;

class LearnItDocConverter
{
public:
	LearnItDocConverter(UTF8OutputStream& out);
	void process(const DocTheory* dt, const std::string& docname);
	void process(const std::vector<const DocTheory*> dts, const std::string docname, std::vector<std::string> variations);
	void addSentenceProcessor(LearnItSentenceConverter_ptr sp); 

	static void indent(int n, std::wostream& output);
private:
	UTF8OutputStream& _output;
	std::vector<LearnItSentenceConverter_ptr> _processors;

	static std::wstring SENTENCE, _SENTENCE;
};

#endif
