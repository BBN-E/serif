// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/patterns/MentionPattern.h"
#include "Generic/patterns/features/MentionPFeature.h"
#include "Generic/common/Symbol.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Sentence.h"
#include "Generic/theories/EntityType.h"
#include "Generic/patterns/Pattern.h"

MentionPFeature::MentionPFeature(Pattern_ptr pattern, const Mention *mention, 
											 Symbol matchSym, const LanguageVariant_ptr& languageVariant,
											 float confidence, bool is_focus):
PatternFeature(pattern, languageVariant, confidence), _mention(mention), _matchSym(matchSym), _is_focus(is_focus)
{
	if (pattern && boost::dynamic_pointer_cast<MentionPattern>(pattern) &&
		boost::dynamic_pointer_cast<MentionPattern>(pattern)->isFocusMentionPattern())
		_is_focus = true;
}

MentionPFeature::MentionPFeature(Pattern_ptr pattern, const Mention *mention, 
											 Symbol matchSym, 
											 float confidence, bool is_focus):
PatternFeature(pattern, confidence), _mention(mention), _matchSym(matchSym), _is_focus(is_focus)
{
	if (pattern && boost::dynamic_pointer_cast<MentionPattern>(pattern) &&
		boost::dynamic_pointer_cast<MentionPattern>(pattern)->isFocusMentionPattern())
		_is_focus = true;
}

bool MentionPFeature::matchesEntityLabel() { 
	return (!_matchSym.is_null() &&
		_matchSym != EntityType::getPERType().getName() && 
		_matchSym != EntityType::getORGType().getName() && 
		_matchSym != EntityType::getGPEType().getName() && 
		_matchSym != EntityType::getLOCType().getName() && 
		_matchSym != EntityType::getFACType().getName() && 
		_matchSym != Symbol(L"VEH") &&
		_matchSym != Symbol(L"WEA"));
}

void MentionPFeature::printFeatureFocus(const PatternMatcher_ptr patternMatcher, UTF8OutputStream &out) const {
	SentenceTheory *sentenceTheory = patternMatcher->getDocTheory()->getSentenceTheory(getSentenceNumber());
	TokenSequence *tSeq = sentenceTheory->getTokenSequence();
	MentionSet *mSet = sentenceTheory->getMentionSet();
	out << L"    <focus type=\"mention\"";
    printOffsetsForSpan(patternMatcher, getSentenceNumber(), getStartToken(), getEndToken(), out);
	out << L" val" << val_sent_no << "=\"" << getSentenceNumber() << L"\"";
	out << L" val" << val_id << "=\"" << _mention->getUID() << L"\"";
	out << L" val" << val_confidence << "=\"" << getConfidence() << L"\"";
	out << L" val" << val_start_token << "=\"" << getStartToken() << L"\"";
	out << L" val" << val_end_token << "=\"" << getEndToken() << L"\"";
	if (!_matchSym.is_null())
		out << L" val" << val_extra << "=\"" << getXMLPrintableString(std::wstring(_matchSym.to_string())) << L"\"";
	else out << L" val" << val_extra << "=\"" << getXMLPrintableString(std::wstring(_mention->getNode()->getHeadWord().to_string())) << L"\"";
	if (MentionPattern_ptr pattern = boost::dynamic_pointer_cast<MentionPattern>(getPattern())) {
		if (pattern->isFocusMentionPattern())
			out << L" val" << val_extra_2 << "=\"FOCUS\"";
	}
	out << L" />\n";
}

void MentionPFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	PatternFeature::saveXML(elem, idMap);

	if (!_matchSym.is_null())
		elem.setAttribute(X_match_sym, _matchSym);
	elem.setAttribute(X_is_focus, _is_focus);

	if (!idMap->hasId(_mention))
		throw UnexpectedInputException("MentionPFeature::saveXML", "Could not find XML ID for Mention");
	elem.setAttribute(X_mention_id, idMap->getId(_mention));
}

MentionPFeature::MentionPFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap)
: PatternFeature(elem, idMap)
{
	using namespace SerifXML;

	if (elem.hasAttribute(X_match_sym))
		_matchSym = elem.getAttribute<Symbol>(X_match_sym);
	else 
		_matchSym = Symbol();
	_is_focus = elem.getAttribute<bool>(X_is_focus);

	_mention = dynamic_cast<const Mention *>(idMap->getTheory(elem.getAttribute<const XMLCh*>(X_mention_id)));
}
