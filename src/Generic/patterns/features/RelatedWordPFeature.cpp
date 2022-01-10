// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/RelatedWordPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/TokenSequence.h"

RelatedWordPFeature::RelatedWordPFeature(int start_token, int end_token, int sent_no, const LanguageVariant_ptr& languageVariant) 
: PseudoPatternFeature(languageVariant), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}

RelatedWordPFeature::RelatedWordPFeature(int start_token, int end_token, int sent_no) 
: PseudoPatternFeature(), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {}

void RelatedWordPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	SentenceTheory *sentenceTheory = patternMatcher->getDocTheory()->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();

	out << L"    <focus type=\"related_word\"";
    printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
	out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";
	out << L" val" << val_start_token << "=\"" << _start_token << L"\"";
	out << L" val" << val_end_token << "=\"" << _end_token << L"\"";
	out << L" />\n";
}

void RelatedWordPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PseudoPatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
}

RelatedWordPFeature::RelatedWordPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PseudoPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
}
