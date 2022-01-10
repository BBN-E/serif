// Copyright (c) 2006 by BBNT Solutions LLC
// All Rights Reserved.

#include "common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/RelMentionPFeature.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/RelMention.h"

RelMentionPFeature::RelMentionPFeature(Pattern_ptr pattern, const RelMention *mention, int sent_no, const LanguageVariant_ptr& languageVariant)
: PatternFeature(pattern,languageVariant), _relMention(mention), _sent_no(sent_no), _start_token(-1), _end_token(-1) 
{}

RelMentionPFeature::RelMentionPFeature(const RelMention *mention, int sent_no, int start_token, int end_token, const LanguageVariant_ptr& languageVariant, float confidence)
: PatternFeature(Pattern_ptr(), languageVariant, confidence), _relMention(mention), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) 
{}

RelMentionPFeature::RelMentionPFeature(Pattern_ptr pattern, const RelMention *mention, int sent_no)
: PatternFeature(pattern), _relMention(mention), _sent_no(sent_no), _start_token(-1), _end_token(-1) 
{}

RelMentionPFeature::RelMentionPFeature(const RelMention *mention, int sent_no, int start_token, int end_token, float confidence)
: PatternFeature(Pattern_ptr(), confidence), _relMention(mention), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) 
{}


void RelMentionPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	const DocTheory* docTheory = patternMatcher->getDocTheory();
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();
	MentionSet *mSet = sentenceTheory->getMentionSet();

	out << L"    <focus type=\"relation\"";
    printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
	out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";
	out << L" val" << val_id << "=\"" << _relMention->getUID().toInt() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << _start_token << L"\"";
	out << L" val" << val_end_token << "=\"" << _end_token << L"\"";
	out << L" val" << val_extra << "=\"" << _relMention->getType() << L"\"";
	out << L" />\n";

	const Mention *leftArgMent = _relMention->getLeftMention();
	const Mention *rightArgMent = _relMention->getRightMention();

	out << L"    <focus type=\"relation_argument\"";
    printOffsetsForSpan(patternMatcher, _sent_no, leftArgMent->getNode()->getStartToken(), leftArgMent->getNode()->getEndToken(), out);
	out << L" val" << val_arg_parent_id << "=\"" << _relMention->getUID().toInt() << L"\"";
	out << L" val" << val_arg_role << "=\"ARG1\"";
	out << L" val" << val_arg_id << "=\"" << leftArgMent->getUID() << L"\"";
	out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(leftArgMent, docTheory)) << L"\"";
	out << L" />\n";
    
	out << L"    <focus type=\"relation_argument\"";
    printOffsetsForSpan(patternMatcher, _sent_no, rightArgMent->getNode()->getStartToken(), rightArgMent->getNode()->getEndToken(), out);
	out << L" val" << val_arg_parent_id << "=\"" << _relMention->getUID().toInt() << L"\"";
	out << L" val" << val_arg_role << "=\"ARG2\"";
	out << L" val" << val_arg_id << "=\"" << rightArgMent->getUID() << L"\"";
	out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(rightArgMent, docTheory)) << L"\"";
	out << L" />\n";
}

void RelMentionPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void RelMentionPFeature::setCoverage(const DocTheory * docTheory) { 

	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);

	// covering node for two relation mention arguments
	_start_token = _relMention->getLeftMention()->getNode()->getStartToken();
	_start_token = std::min(_start_token, _relMention->getRightMention()->getNode()->getStartToken());

	_end_token = _relMention->getLeftMention()->getNode()->getEndToken();
	_end_token = std::max(_end_token, _relMention->getRightMention()->getNode()->getEndToken());
}

void RelMentionPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	if (!idMap->hasId(_relMention))
		throw UnexpectedInputException("RelMentionPFeature::saveXML", "Could not find XML ID for relMention");
	elem.setAttribute(X_rel_mention_id, idMap->getId(_relMention));
}

RelMentionPFeature::RelMentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);

	_relMention = dynamic_cast<const RelMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_rel_mention_id)));
}
