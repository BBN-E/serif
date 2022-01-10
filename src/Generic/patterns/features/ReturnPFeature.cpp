// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/ReturnPFeature.h"
#include "Generic/patterns/PatternReturn.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/Pattern.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"

ReturnPatternFeature::ReturnPatternFeature(Pattern_ptr pattern, const LanguageVariant_ptr& languageVariant, float confidence): 
	PatternFeature(pattern, languageVariant, confidence), _end_token(-1),
	_return(boost::make_shared<PatternReturn>(pattern->getReturn())),
	_toplevel(pattern->isReturnValueTopLevel())
{
}

ReturnPatternFeature::ReturnPatternFeature(Symbol returnLabel, const LanguageVariant_ptr& languageVariant, float confidence): 
	PatternFeature(Pattern_ptr(), languageVariant, confidence), _end_token(-1),
	_return(boost::make_shared<PatternReturn>(returnLabel)),
	_toplevel(false)
{
}

ReturnPatternFeature::ReturnPatternFeature(Pattern_ptr pattern, float confidence): 
	PatternFeature(pattern, confidence), _end_token(-1),
	_return(boost::make_shared<PatternReturn>(pattern->getReturn())),
	_toplevel(pattern->isReturnValueTopLevel())
{
}

ReturnPatternFeature::ReturnPatternFeature(Symbol returnLabel, float confidence): 
	PatternFeature(Pattern_ptr(), confidence), _end_token(-1),
	_return(boost::make_shared<PatternReturn>(returnLabel)),
	_toplevel(false)
{
}

void ReturnPatternFeature::setPatternReturn(const PatternReturn_ptr patternReturn) {
	_return = boost::make_shared<PatternReturn>(patternReturn);
}

void ReturnPatternFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void ReturnPatternFeature::setCoverage(const DocTheory * docTheory) {
	// Set the end token to the end of the sentence.  Note: this is used by all
	// return pattern features *except* TokenSpanReturnPFeature.
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(getSentenceNumber());
	_end_token = sentenceTheory->getTokenSequence()->getNTokens() - 1;
}

Symbol ReturnPatternFeature::getReturnLabel() const { 
	return _return->getLabel(); 
}

std::wstring ReturnPatternFeature::getReturnValue(const std::wstring & key) const {
	return _return->getValue(key);
}

bool ReturnPatternFeature::hasReturnValue(const std::wstring & key) const {
	return _return->hasValue(key);
}

void ReturnPatternFeature::setReturnValue(const std::wstring & key, const std::wstring & value) {
	_return->setValue(key, value);
}

std::map<std::wstring, std::wstring>::const_iterator ReturnPatternFeature::begin() const { 
	return _return->begin(); 
}

std::map<std::wstring, std::wstring>::const_iterator ReturnPatternFeature::end() const { 
	return _return->end(); 
}

void ReturnPatternFeature::printFeatureFocusHeader(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	out << L"    <focus type=\"return\"";
    printOffsetsForSpan(patternMatcher, getSentenceNumber(), getStartToken(), getEndToken(), out);
	out << L" val" << val_sent_no << "=\"" << getSentenceNumber() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << getStartToken() << L"\"";
	out << L" val" << val_end_token << "=\"" << getEndToken() << L"\"";
	out << L" val" << val_extra << "=\"" << getXMLPrintableString(std::wstring(getPattern()->getReturnLabelFromPattern().to_string())) << L"\"";
}

void ReturnPatternFeature::printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value) const {
	printFeatureFocusHeader(patternMatcher, out);
	out << L" val" << val_extra_2 << "=\"" << val_extra_2_value << L"\"";
	out << L" />\n";
}

void ReturnPatternFeature::printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value, 
												   int val_extra_3_value) const
{
	printFeatureFocusHeader(patternMatcher, out);
	out << L" val" << val_extra_2 << "=\"" << val_extra_2_value << L"\"";
	out << L" val" << val_extra_3 << "=\"" << val_extra_3_value << L"\"";
	out << L" />\n";
}

void ReturnPatternFeature::printFeatureFocusHelper(PatternMatcher_ptr patternMatcher, UTF8OutputStream &out, const Symbol& val_extra_2_value, 
												   float val_extra_3_value) const 
{
	printFeatureFocusHeader(patternMatcher, out);
	out << L" val" << val_extra_2 << "=\"" << val_extra_2_value << L"\"";
	out << L" val" << val_extra_3 << "=\"" << val_extra_3_value << L"\"";
	out << L" />\n";
}

void ReturnPatternFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_end_token, _end_token);
	elem.setAttribute(X_toplevel, _toplevel);

	XMLElement patternReturnElem = elem.addChild(X_PatternReturn);
	_return->saveXML(patternReturnElem);
}

ReturnPatternFeature::ReturnPatternFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_end_token = elem.getAttribute<int>(X_end_token);
	_toplevel = elem.getAttribute<bool>(X_toplevel);

	std::vector<XMLElement> patternReturnVector = elem.getChildElementsByTagName(X_PatternReturn);
	if (patternReturnVector.size() > 1)
		throw UnexpectedInputException("ReturnPatternFeature::ReturnPatternFeature", "more than one PatternReturn");
	if (patternReturnVector.size() == 1)
		_return = boost::make_shared<PatternReturn>(patternReturnVector[0]);
}

void MentionReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);

	if (!_matchSym.is_null())
		elem.setAttribute(X_match_sym, _matchSym);
	
	elem.setAttribute(X_is_focus, _is_focus);

	if (!idMap->hasId(_mention)) {
		throw UnexpectedInputException("MentionReturnPFeature::saveXML", "Could not find XML ID for Mention");
	}
	elem.setAttribute(X_mention_id, idMap->getId(_mention));
}

MentionReturnPFeature::MentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_match_sym))
		_matchSym = elem.getAttribute<Symbol>(X_match_sym);
	
	_is_focus = elem.getAttribute<bool>(X_is_focus);
	_mention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_mention_id)));
}

void ValueMentionReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);

	if (!idMap->hasId(_valueMention))
		throw UnexpectedInputException("ValueMentionReturnPFeature::saveXML", "Could not find XML ID for ValueMention");
	elem.setAttribute(X_value_mention_id, idMap->getId(_valueMention));
}

ValueMentionReturnPFeature::ValueMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_valueMention = dynamic_cast<const ValueMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_value_mention_id)));
}

void DocumentDateReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_formatted_date, _formattedDate);
	elem.setAttribute(X_sentence_number, _sent_no);
}

DocumentDateReturnPFeature::DocumentDateReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_formattedDate = elem.getAttribute<std::wstring>(X_formatted_date);
	_sent_no = elem.getAttribute<int>(X_sentence_number);
}

void TopicReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_relevance_score, _relevance_score);
	elem.setAttribute(X_query_slot, _querySlot);

	if (!idMap->hasId(_mention)) {
		throw UnexpectedInputException("TopicReturnPFeature::saveXML", "Could not find XML ID for Mention");
	}
	elem.setAttribute(X_mention_id, idMap->getId(_mention));
}

TopicReturnPFeature::TopicReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_relevance_score = elem.getAttribute<float>(X_relevance_score);
	_querySlot = elem.getAttribute<Symbol>(X_query_slot);
	_mention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_mention_id)));
}

void TokenSpanReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);
}

TokenSpanReturnPFeature::TokenSpanReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
}

void PropositionReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);

	if (!idMap->hasId(_prop))
		throw UnexpectedInputException("PropositionReturnPFeature::saveXML", "Could not find XML ID for Proposition");
	elem.setAttribute(X_proposition_id, idMap->getId(_prop));
}

PropositionReturnPFeature::PropositionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_prop = dynamic_cast<const Proposition *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_proposition_id)));
}

void ParseNodeReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);

	if (!idMap->hasId(_node))
		throw UnexpectedInputException("ParseNodeReturnPFeature::saveXML", "Could not find XML ID for SynNode");
	elem.setAttribute(X_syn_node_id, idMap->getId(_node));
}

ParseNodeReturnPFeature::ParseNodeReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_node = dynamic_cast<const SynNode *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_syn_node_id)));
}

void DateSpecReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_spec_string, _specString);

	if (!idMap->hasId(_valueMention1))
		throw UnexpectedInputException("DateSpecReturnPFeature::saveXML", "Could not find XML ID for ValueMention1");
	elem.setAttribute(X_value_mention_1_id, idMap->getId(_valueMention1));

	if (!idMap->hasId(_valueMention2))
		throw UnexpectedInputException("DateSpecReturnPFeature::saveXML", "Could not find XML ID for ValueMention2");
	elem.setAttribute(X_value_mention_2_id, idMap->getId(_valueMention2));
}

DateSpecReturnPFeature::DateSpecReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_specString = elem.getAttribute<std::wstring>(X_spec_string);

	_valueMention1 = dynamic_cast<const ValueMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_value_mention_1_id)));
	_valueMention2 = dynamic_cast<const ValueMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_value_mention_2_id)));
}

void GenericReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);
}

GenericReturnPFeature::GenericReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
}

void EventMentionReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);

	if (!idMap->hasId(_eventMention)) {
		throw UnexpectedInputException("EventMentionReturnPFeature::saveXML", "Could not find XML ID for EventMention");
	}
	elem.setAttribute(X_event_mention_id, idMap->getId(_eventMention));
}

EventMentionReturnPFeature::EventMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_eventMention = dynamic_cast<const EventMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_event_mention_id)));
}

void RelMentionReturnPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	ReturnPatternFeature::saveXML(elem, idMap);
	elem.setAttribute(X_sentence_number, _sent_no);

	if (!idMap->hasId(_relMention)) {
		throw UnexpectedInputException("RelMentionReturnPFeature::saveXML", "Could not find XML ID for RelMention");
	}
	elem.setAttribute(X_rel_mention_id, idMap->getId(_relMention));
}

RelMentionReturnPFeature::RelMentionReturnPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: ReturnPatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_relMention = dynamic_cast<const RelMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_rel_mention_id)));
}
