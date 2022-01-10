// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/EventMentionPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/EventMention.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/Parse.h"
#include "Generic/theories/SynNode.h"

EventMentionPFeature::EventMentionPFeature(Pattern_ptr pattern, const EventMention *vm, int sent_no, const LanguageVariant_ptr& languageVariant) :
PatternFeature(pattern,languageVariant), _eventMention(vm), _sent_no(sent_no), _start_token(-1), _end_token(-1)
{
}

EventMentionPFeature::EventMentionPFeature(const EventMention *vm, int sent_no, int start_token, int end_token, 
										   const LanguageVariant_ptr& languageVariant, float confidence): 
PatternFeature(Pattern_ptr(), languageVariant, confidence), _sent_no(sent_no), _start_token(start_token), _end_token(end_token) {
}

void EventMentionPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void EventMentionPFeature::setCoverage(const DocTheory * docTheory) { 

	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	
	const SynNode *node = _eventMention->getAnchorNode();
	int event_start = node->getStartToken();
	int event_end = node->getEndToken();

	for (int a = 0; a < _eventMention->getNArgs(); a++) {
		const Mention *ment = _eventMention->getNthArgMention(a);
		event_start = std::min(event_start, ment->getNode()->getStartToken());
		event_end = std::max(event_end, ment->getNode()->getEndToken());
	}
	for (int v = 0; v < _eventMention->getNValueArgs(); v++) {
		const ValueMention *vment = _eventMention->getNthArgValueMention(v);		
		event_start = std::min(event_start, vment->getStartToken());
		event_end = std::max(event_end, vment->getEndToken());
	}

	node = sentenceTheory->getPrimaryParse()->getRoot()->getCoveringNodeFromTokenSpan(event_start, event_end);
	
	// if this is a noun phrase, get highest noun phrase headed by our node
	// if not, get lowest S/SBAR covering our node

	Symbol tag = node->getHeadPreterm()->getTag();
	LanguageAttribute language = docTheory->getDocument()->getLanguage();

	if (SynNode::isNounTag(tag, language)) {
		// walk up head chain
		while (node->getParent() != 0 && node->getParent()->getHead() == node)
			node = node->getParent();
	} else {
		// walk up to the nearest S or SBAR
		while (node->getParent() != 0 && !SynNode::isSentenceLikeTag(node->getTag(), language))
			node = node->getParent();
	}

	_start_token = node->getStartToken();
	_end_token = node->getEndToken();
}

void EventMentionPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	const DocTheory* docTheory = patternMatcher->getDocTheory();
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();
	MentionSet *mSet = sentenceTheory->getMentionSet();

	out << L"    <focus type=\"event\"";
    printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
	out << L" val" << val_sent_no << "=\"" << getSentenceNumber() << L"\"";
	out << L" val" << val_id << "=\"" << _eventMention->getUID().toInt() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << getStartToken() << L"\"";
	out << L" val" << val_end_token << "=\"" << getEndToken() << L"\"";
	out << L" val" << val_extra << "=\"" << _eventMention->getEventType() << L"\"";
	out << L" />\n";

	for (int arg = 0; arg < _eventMention->getNArgs(); arg++) {
		const Mention *argMent = _eventMention->getNthArgMention(arg);
		out << L"    <focus type=\"event_argument\"";
        printOffsetsForSpan(patternMatcher, _sent_no, argMent->getNode()->getStartToken(), argMent->getNode()->getEndToken(), out);
		out << L" val" << val_arg_parent_id << "=\"" << _eventMention->getUID().toInt() << L"\" ";
		out << L" val" << val_arg_role << "=\"" << _eventMention->getNthArgRole(arg) << L"\"";
		out << L" val" << val_arg_id << "=\"" << argMent->getUID() << L"\"";
		out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(argMent, docTheory)) << L"\"";
		out << L" />\n";
	}		

	for (int varg = 0; varg < _eventMention->getNValueArgs(); varg++) {
		const ValueMention *argValueMent = _eventMention->getNthArgValueMention(varg);
		out << L"    <focus type=\"event_argument\"";
        printOffsetsForSpan(patternMatcher, _sent_no, argValueMent->getStartToken(), argValueMent->getEndToken(), out);
		out << L" val" << val_arg_parent_id << "=\"" << _eventMention->getUID().toInt() << L"\"";
		out << L" val" << val_arg_role << "=\"" << getXMLPrintableString(std::wstring(_eventMention->getNthArgValueRole(varg).to_string())) << L"\"";
		out << L" val" << val_arg_id << "=\"" << argValueMent->getUID().toInt() << L"\"";
		if (argValueMent->isTimexValue()) {				
			Symbol timexVal = Symbol(L"TODO");
			out << L" val" << val_arg_canonical_ref << "=\"";
			out << timexVal;
			out << L"\"";
		} else {
			out << L" val" << val_arg_canonical_ref << "=\"" << argValueMent->getType() << L"\"";
		}
		out << L" />\n";
	}
}

void EventMentionPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	if (!idMap->hasId(_eventMention))
		throw UnexpectedInputException("EventMentionPFeature::saveXML", "Could not find XML ID for EventMention");

	elem.setAttribute(X_event_mention_id, idMap->getId(_eventMention));
}

EventMentionPFeature::EventMentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);
	
	_eventMention = dynamic_cast<const EventMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_event_mention_id)));
}
