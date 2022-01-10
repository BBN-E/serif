// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/features/ValueMentionPFeature.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/ValueMention.h"
#include "Generic/patterns/Pattern.h"

ValueMentionPFeature::ValueMentionPFeature(Pattern_ptr pattern, const ValueMention *valueMention, Symbol matchSym, 
										   const LanguageVariant_ptr& languageVariant, float confidence) 
 : PatternFeature(pattern, languageVariant, confidence), _valueMention(valueMention), _matchSym(matchSym), _is_focus(false) {}
 
 ValueMentionPFeature::ValueMentionPFeature(Pattern_ptr pattern, const ValueMention *valueMention, Symbol matchSym, 
										   float confidence) 
 : PatternFeature(pattern, confidence), _valueMention(valueMention), _matchSym(matchSym), _is_focus(false) {}


void ValueMentionPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {

	SentenceTheory *sentenceTheory = patternMatcher->getDocTheory()->getSentenceTheory(getSentenceNumber());
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();

	out << L"    <focus type=\"value\"";
    printOffsetsForSpan(patternMatcher, getSentenceNumber(), getStartToken(), getEndToken(), out);
	out << L" val" << val_sent_no << "=\"" << getSentenceNumber() << L"\"";
	out << L" val" << val_id << "=\"" << _valueMention->getUID().toInt() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << getStartToken() << L"\"";
	out << L" val" << val_end_token << "=\"" << getEndToken() << L"\"";
	out << L" val" << val_extra << "=\"" << _valueMention->getType()<< L"\"";
	out << L" />\n";
}

void ValueMentionPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	if (!_matchSym.is_null())
		elem.setAttribute(X_match_sym, _matchSym);
	elem.setAttribute(X_is_focus, _is_focus);

	if (!idMap->hasId(_valueMention)) 
		throw UnexpectedInputException("ValueMentionPFeature::saveXML", "Could not find XML ID for valueMention");
	elem.setAttribute(X_value_mention_id, idMap->getId(_valueMention));
}

ValueMentionPFeature::ValueMentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_match_sym))
		_matchSym = elem.getAttribute<Symbol>(X_match_sym);
	else
		_matchSym = Symbol();
	_is_focus = elem.getAttribute<bool>(X_is_focus);

	_valueMention = dynamic_cast<const ValueMention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_value_mention_id)));
}
