// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/values/PatternValueFinder.h"
#include "Generic/values/ValueRecognizer.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/RegexPattern.h"
#include "Generic/patterns/features/PatternFeatureSet.h"
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/common/Sexp.h"

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>

PatternValueFinder::PatternValueFinder() : _docTheory(0) {

	std::string pattern_set_list = ParamReader::getRequiredParam("value_pattern_set_list");
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(pattern_set_list));
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::wstring line;
	while (stream) {
		stream.getLine(line);
		if (!stream) 
			break;
		if (line.size()>0 && line[0]!=L'#') {
			std::string s = UnicodeUtil::toUTF8StdString(line);
			s = ParamReader::expand(s);
			PatternSet_ptr ps = boost::make_shared<PatternSet>(s.c_str());
			_patternSets.push_back(ps);
		}
	}
	stream.close();

	BOOST_FOREACH(PatternSet_ptr ps, _patternSets) 
		if (!ps->validForPreParseMatching()) 
			throw UnexpectedInputException("PatternValueFinder::PatternValueFinder", "Only regex patterns with text pattern children allowed in pattern file");
}

PatternValueFinder::~PatternValueFinder() {}

void PatternValueFinder::resetForNewDocument(DocTheory *docTheory) {
	_docTheory = docTheory;
}

ValueRecognizer::SpanList PatternValueFinder::findValueMentions(int sent_no) {

	if (_docTheory == 0)
		throw InternalInconsistencyException("PatternValueFinder::findValueMentions", "DocTheory not set");

	ValueRecognizer::SpanList results;

	SentenceTheory *sTheory = _docTheory->getSentenceTheory(sent_no);

	std::vector<PatternFeatureSet_ptr> all_results;
	BOOST_FOREACH(PatternSet_ptr ps, _patternSets) {
		PatternMatcher_ptr pm = PatternMatcher::makePatternMatcher(_docTheory, ps);
		std::vector<PatternFeatureSet_ptr> results = pm->getSentenceSnippets(sTheory, 0, true);
		all_results.insert(all_results.end(), results.begin(), results.end());
	}

	BOOST_FOREACH(PatternFeatureSet_ptr pfs, all_results) {
		for (size_t f = 0; f < pfs->getNFeatures(); f++) {
			PatternFeature_ptr feat = pfs->getFeature(f);
			if (ReturnPatternFeature_ptr rf = boost::dynamic_pointer_cast<ReturnPatternFeature>(feat)) {
				Symbol valueTypeSym = rf->getReturnValue(L"type");

				if (!ValueType::isValidValueType(valueTypeSym))
					throw UnexpectedInputException("PatternValueFinder::findValueMentions", "Bad value type in pattern file");

				if (TokenSpanReturnPFeature_ptr tsrf = boost::dynamic_pointer_cast<TokenSpanReturnPFeature>(rf)) {
					int start_token = tsrf->getStartToken();
					int end_token = tsrf->getEndToken();

					ValueRecognizer::ValueSpan span(start_token, end_token, valueTypeSym);
					results.push_back(span);
				} 
			}
		}
	}

	return results;
}
