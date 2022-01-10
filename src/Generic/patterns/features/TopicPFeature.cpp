// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/TopicPFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/TopicPattern.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/SynNode.h"
#include "Generic/PropTree/DocPropForest.h"


TopicPFeature::TopicPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention *mention, 
							 const LanguageVariant_ptr& languageVariant)
: PatternFeature(pattern,languageVariant), _sent_no(sent_no), _relevance_score(relevance), _mention(mention), _querySlot(querySlot),
  _start_token(-1), _end_token(-1) {
	if (!boost::dynamic_pointer_cast<TopicPattern>(getPattern()))
		throw InternalInconsistencyException("TopicPFeature::TopicPFeature",
			"TopicPFeature's first argument must be a TopicPattern!");
}

TopicPFeature::TopicPFeature(Pattern_ptr pattern, int sent_no, float relevance, Symbol querySlot, const Mention *mention)
: PatternFeature(pattern), _sent_no(sent_no), _relevance_score(relevance), _mention(mention), _querySlot(querySlot),
  _start_token(-1), _end_token(-1) {
	if (!boost::dynamic_pointer_cast<TopicPattern>(getPattern()))
		throw InternalInconsistencyException("TopicPFeature::TopicPFeature",
			"TopicPFeature's first argument must be a TopicPattern!");
}


void TopicPFeature::setCoverage(const PatternMatcher_ptr patternMatcher) {
	setCoverage(patternMatcher->getDocTheory());
}

void TopicPFeature::setCoverage(const DocTheory * docTheory) { 
	if (_mention) {
		_start_token = _mention->getNode()->getStartToken();
		_end_token = _mention->getNode()->getEndToken();
	} else {
		SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
		_start_token = 0;
		_end_token = sentenceTheory->getTokenSequence()->getNTokens() - 1;
	}
}

void TopicPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	SentenceTheory *sentenceTheory = patternMatcher->getDocTheory()->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();

	// We know this is safe because we checked it in the constructor:
	TopicPattern_ptr topicPattern = boost::dynamic_pointer_cast<TopicPattern>(getPattern());

	std::wstring strategy = L"";
	if (topicPattern->getMatchStrategy() == TopicPattern::FULL)
		strategy = L"full";
	else if (topicPattern->getMatchStrategy() == TopicPattern::EDGE)
		strategy = L"subtree";
	else if (topicPattern->getMatchStrategy() == TopicPattern::NODE)
		strategy = L"node";
	
	out << L"    <focus type=\"topic\"";
	out << L" val" << val_topic_sent_no << "=\"" << _sent_no << L"\"";	
	out << L" val" << val_topic_strategy << "=\"" << strategy << L"\"";	
	out << L" val" << val_topic_score << L"=\"" << _relevance_score << "\"";
	out << L" val" << val_topic_query_slot << "=\"" << getXMLPrintableString(std::wstring(topicPattern->getQuerySlot().to_string())) <<"\"";
	out << L" val" << val_topic_context << "=\"" << topicPattern->getContextSize() <<"\"";
	out << L" val" << val_topic_backward_range << "=\"" << topicPattern->getBackwardRange() <<"\"";
	out << L" val" << val_topic_forward_range << "=\"" << topicPattern->getForwardRange() <<"\"";
	out << L" />\n";
	
	if (_mention != 0) {			
		out << L"    <focus type=\"mention_topic_match\" ";
        printOffsetsForSpan(patternMatcher, _sent_no, _mention->getNode()->getStartToken(), _mention->getNode()->getEndToken(), out);
		out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";	
		out << L" val" << val_id << "=\"" << _mention->getUID() << L"\"";
		out << L" val" << val_score << L"=\"" << _relevance_score << "\"";
		out << L" val" << val_start_token << "=\"" << _mention->getNode()->getStartToken() << L"\"";
		out << L" val" << val_end_token << "=\"" << _mention->getNode()->getEndToken() << L"\"";
		out << L" val" << val_extra_2 << "=\"" << strategy << L"\"";	
		out << L" val" << val_extra_4 << "=\"" << getXMLPrintableString(std::wstring(topicPattern->getQuerySlot().to_string())) <<"\"";
		out << L" />\n";
		out << L" ";
	}	
}

void TopicPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_relevance_score, _relevance_score);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	if (!idMap->hasId(_mention)) 
		throw UnexpectedInputException("TopicPFeature::saveXML", "Could not find XML ID for mention");
	elem.setAttribute(X_mention_id, idMap->getId(_mention));
}

TopicPFeature::TopicPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_relevance_score = elem.getAttribute<float>(X_relevance_score);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);

	_mention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_mention_id)));
}
