// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/patterns/features/ExactMatchPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"

void ExactMatchPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
		out << L"    <focus type=\"exact_match\"";
		out << L" />\n";
}

void ExactMatchPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void ExactMatchPFeature::setCoverage(const DocTheory * docTheory) {
	// Why use the parse rather than just the token sequence?
	const SynNode *_coveringNode = docTheory->getSentenceTheory(_sent_no)->getPrimaryParse()->getRoot();
	_start_token = _coveringNode->getStartToken(); // won't this always be zero?
	_end_token = _coveringNode->getEndToken();
}

bool ExactMatchPFeature::equals(PatternFeature_ptr other) {
	boost::shared_ptr<ExactMatchPFeature> f = 
		boost::dynamic_pointer_cast<ExactMatchPFeature>(other);
	return PatternFeature::simpleEquals(other) && f;
}

void ExactMatchPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PseudoPatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
}

ExactMatchPFeature::ExactMatchPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PseudoPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
}
