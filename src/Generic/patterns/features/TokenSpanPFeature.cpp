// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/patterns/features/TokenSpanPFeature.h"
#include "Generic/patterns/PatternMatcher.h"

void TokenSpanPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	out << L"    <focus type=\"token_span\"";
    printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
	out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";
	out << L" val" << val_start_token << "=\"" << _start_token << L"\"";
	out << L" val" << val_end_token << "=\"" << _end_token << L"\"";
	out << L" />\n";
}

void TokenSpanPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PseudoPatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
}

TokenSpanPFeature::TokenSpanPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PseudoPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
}
