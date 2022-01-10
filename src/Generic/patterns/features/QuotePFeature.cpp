// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/QuotePFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"

namespace { // Private symbols
	Symbol irrelevantQuotationSym(L"irrelevant_quotation_segment");
	Symbol relevantQuotationSym(L"relevent_quotation_segment");
	Symbol quotationSpeakerSym(L"quotation_speaker");
}

QuotePFeature::QuotePFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token, bool relevant, const LanguageVariant_ptr& languageVariant): 
PatternFeature(pattern,languageVariant), _sent_no(sent_no), _start_token(start_token), _end_token(end_token),
_focusType(relevant?relevantQuotationSym:irrelevantQuotationSym), _speakerMention(0) {}

QuotePFeature::QuotePFeature(Pattern_ptr pattern, const Mention* mention, const LanguageVariant_ptr& languageVariant): 
PatternFeature(pattern,languageVariant), _sent_no(mention->getSentenceNumber()), 
_start_token(mention->getNode()->getStartToken()), _end_token(mention->getNode()->getEndToken()),
_focusType(quotationSpeakerSym), _speakerMention(mention) {}

QuotePFeature::QuotePFeature(Pattern_ptr pattern, int sent_no, int start_token, int end_token, bool relevant): 
PatternFeature(pattern), _sent_no(sent_no), _start_token(start_token), _end_token(end_token),
_focusType(relevant?relevantQuotationSym:irrelevantQuotationSym), _speakerMention(0) {}

QuotePFeature::QuotePFeature(Pattern_ptr pattern, const Mention* mention): 
PatternFeature(pattern), _sent_no(mention->getSentenceNumber()), 
_start_token(mention->getNode()->getStartToken()), _end_token(mention->getNode()->getEndToken()),
_focusType(quotationSpeakerSym), _speakerMention(mention) {}

void QuotePFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	const DocTheory* docTheory = patternMatcher->getDocTheory();
	SentenceTheory *sentenceTheory = docTheory->getSentenceTheory(_sent_no);
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();

	out << L"    <focus type=\"" << _focusType << "\" ";
	if (_focusType == quotationSpeakerSym) {
        printOffsetsForSpan(patternMatcher, _sent_no, _speakerMention->getNode()->getStartToken(), _speakerMention->getNode()->getEndToken(), out);
		out << L" val" << val_arg_id << "=\"" << _speakerMention->getUID() << L"\"";
		out << L" val" << val_arg_canonical_ref << "=\"" << getXMLPrintableString(getBestNameForMention(_speakerMention, docTheory)) << L"\"";
	} else {
        printOffsetsForSpan(patternMatcher, _sent_no, _start_token, _end_token, out);
		out << L" val" << val_sent_no << "=\"" << _sent_no << L"\"";
		out << L" val" << val_start_token << "=\"" << _start_token << L"\"";
		out << L" val" << val_end_token << "=\"" << _end_token << L"\"";
	}
	

	out << L" />\n";
}

bool QuotePFeature::equals(PatternFeature_ptr other) {
	boost::shared_ptr<QuotePFeature> f = boost::dynamic_pointer_cast<QuotePFeature>(other);
	return PatternFeature::simpleEquals(other) && 
		f && f->getSpeakerMention() == getSpeakerMention() &&
		f->getFocusType() == getFocusType();
}

void QuotePFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	if (!_focusType.is_null())
		elem.setAttribute(X_focus_type, _focusType);
	elem.setAttribute(X_sentence_number, _sent_no);
	elem.setAttribute(X_start_token, _start_token);
	elem.setAttribute(X_end_token, _end_token);

	if (!idMap->hasId(_speakerMention)) 
		throw UnexpectedInputException("QuotePFeature::saveXML", "Could not find XML ID for speaker Mention");
	elem.setAttribute(X_speaker_mention_id, idMap->getId(_speakerMention));
}

QuotePFeature::QuotePFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_focus_type))
		_focusType = elem.getAttribute<Symbol>(X_focus_type);
	else
		_focusType = Symbol();

	_sent_no = elem.getAttribute<int>(X_sentence_number);
	_start_token = elem.getAttribute<int>(X_start_token);
	_end_token = elem.getAttribute<int>(X_end_token);

	_speakerMention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_speaker_mention_id)));
}
