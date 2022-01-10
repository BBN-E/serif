// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/patterns/features/PatternFeature.h"
#include "Generic/patterns/PatternMatcher.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/SentenceTheory.h"
#include "Generic/theories/TokenSequence.h"
#include "Generic/theories/Token.h"
#include <boost/algorithm/string/replace.hpp>

void PatternFeature::printOffsetsForSpan(const PatternMatcher_ptr patternMatcher, int sentenceNumber, int startTokenIndex, 
										 int endTokenIndex, UTF8OutputStream &out) const {
	TokenSequence* tokenSequence = patternMatcher->getDocTheory()->getSentenceTheory(sentenceNumber)->getTokenSequence();

    EDTOffset spanStartOffset, spanEndOffset;
	spanStartOffset = tokenSequence->getToken(startTokenIndex)->getStartEDTOffset();
    spanEndOffset = tokenSequence->getToken(endTokenIndex)->getEndEDTOffset();

    out << L" start=\"" << spanStartOffset.value() << L"\" ";  // Leading space because we are writing out an attribute in a list of attributes
    out << L"end=\"" << spanEndOffset.value() << L"\"";
}

std::wstring PatternFeature::getBestNameForMention(const Mention *mention, const DocTheory *docTheory) const {
	if(!mention->getEntityType().isRecognized()){
		return L"NON_ACE";
	}
	Entity* ent = docTheory->getEntitySet()->getEntityByMention(mention->getUID());
	if(ent != 0){
		return ent->getBestName(docTheory);
	} else {
		return L"NO_NAME";
	}	
}

bool PatternFeature::simpleEquals(PatternFeature_ptr other) {
	return other->getSentenceNumber() == getSentenceNumber() &&
		other->getStartToken() == getStartToken() &&
		other->getEndToken() == getEndToken();
}

void PatternFeature::saveXML(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) const {
	using namespace SerifXML;

	elem.setAttribute(X_confidence, _confidence);
	if (!_languageVariant->getLanguage().is_null()) {
		elem.setAttribute(X_feature_language, _languageVariant->getLanguage());
	}
	if (!_languageVariant->getVariant().is_null()) {
		elem.setAttribute(X_feature_variant, _languageVariant->getVariant());
	}
}

PatternFeature::PatternFeature(SerifXML::XMLElement elem, const SerifXML::XMLIdMap* idMap) :
	_languageVariant(LanguageVariant::getLanguageVariant(elem.getAttribute<Symbol>(SerifXML::X_feature_language),
		elem.getAttribute<Symbol>(SerifXML::X_feature_variant)))
{ 
	_confidence = elem.getAttribute<float>(SerifXML::X_confidence);
}
