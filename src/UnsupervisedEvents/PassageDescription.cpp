#include "Generic/common/leak_detection.h"
#include "PassageDescription.h"
#include <iostream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"

using std::vector;
using std::wstring;
using std::string;
using std::wifstream;
using std::getline;
using std::wstringstream;
using boost::split;
using boost::is_any_of;
using boost::make_shared;
using boost::lexical_cast;

PassageDescriptions_ptr PassageDescription::loadPassageDescriptions(
		const string& filename)
{
	wifstream inp(filename.c_str());
	wstring line;
	vector<wstring> parts;
	PassageDescriptions_ptr passageDescriptions = make_shared<PassageDescriptions>();

	while(getline(inp, line)) {
		split(parts, line, is_any_of(L"\t"));
		if (parts.size() != 3) {
			SessionLogger::warn("bad_passage_description")
				<< L"Bad passage description line \"" << line << L"\", skipping.";
			continue;
		}

		passageDescriptions->insert(make_pair(parts[0], 
					make_shared<PassageDescription>(parts[0], 
				lexical_cast<unsigned int>(parts[1]), 
				lexical_cast<unsigned int>(parts[2]))));
	}

	return passageDescriptions;
}

void PassageDescription::attachDocTheory(const DocTheory* dt) {
	if (startSentence() < 0 || endSentence() < startSentence() 
			|| endSentence() >= (unsigned int)dt->getNSentences())
	{
		wstringstream err;
		err << L"Passage indices out of bounds for docid = " << docid()
			<< L", start=" << startSentence() << L", end=" << endSentence()
			<< L"; num sentences in doc is " << dt->getNSentences() ;
		throw UnexpectedInputException("PassageDescription::attachToDocTheory", err);
	}

	wstringstream text;

	for (unsigned int i=startSentence(); i<endSentence(); ++i) {
		const SentenceTheory* st = dt->getSentenceTheory(i);
		const TokenSequence* ts = st->getTokenSequence();
		text << ts->toString();
	}

	_passage = text.str();
}


