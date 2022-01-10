// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Parse.h"
#include "Generic/causeEffect/CauseEffectRelation.h"

CauseEffectRelation::CauseEffectRelation(const Proposition *cause, const Proposition *effect, const SentenceTheory *sentTheory, Symbol relationType) 
	:_cause(cause), _effect(effect), _causeSentenceTheory(sentTheory), _effectSentenceTheory(sentTheory), _relationType(relationType)
{ }

const Proposition* CauseEffectRelation::getCause() {
	return _cause;
}

const SentenceTheory* CauseEffectRelation::getCauseSentenceTheory() {
	return _causeSentenceTheory;
}

const Proposition* CauseEffectRelation::getEffect() {
	return _effect;
}

const SentenceTheory* CauseEffectRelation::getEffectSentenceTheory() {
	return _effectSentenceTheory;
}

const Symbol CauseEffectRelation::getRelationType() {
	return _relationType;
}

int CauseEffectRelation::getPropStartOffset(const Proposition* prop) {
	int predStartTokenIndex = prop->getPredHead()->getStartToken();
	const Token *predStartToken = _causeSentenceTheory->getTokenSequence()->getToken(predStartTokenIndex);
	return predStartToken->getStartCharOffset().value();
}

int CauseEffectRelation::getPropEndOffset(const Proposition* prop) {
	int predEndTokenIndex = prop->getPredHead()->getEndToken();
	const Token *predEndToken = _causeSentenceTheory->getTokenSequence()->getToken(predEndTokenIndex);
	return predEndToken->getEndCharOffset().value();
}

std::wstring CauseEffectRelation::getJsonString(Symbol docid) {
	std::wstringstream wss;

	int cause_start_offset = getPropStartOffset(_cause);
	int cause_end_offset = getPropEndOffset(_cause);
	
	int effect_start_offset = getPropStartOffset(_effect);
	int effect_end_offset = getPropEndOffset(_effect);

	wss << L"        {\n";
	wss << L"            \"arg1_span_list\": [ [" << cause_start_offset << L", " << cause_end_offset << L"] ],\n";
	wss << L"            \"arg2_span_list\": [ [" << effect_start_offset << L", " << effect_end_offset << L"] ],\n";
	wss << L"            \"arg1_text\": \"" << escapeJsonString(_cause->getPredHead()->toCasedTextString(_causeSentenceTheory->getTokenSequence())) << "\",\n";
	wss << L"            \"arg2_text\": \"" << escapeJsonString(_effect->getPredHead()->toCasedTextString(_effectSentenceTheory->getTokenSequence())) << "\",\n";
	wss << L"            \"docid\": \"" << docid << "\",\n";
	wss << L"            \"relation_type\": \"Explicit\",\n";
	wss << L"            \"semantic_class\": \"" << _relationType.to_string() << "\",\n";
	wss << L"            \"model\": \"SERIF\",\n";
	wss << L"            \"cause_sentence\": \"" << escapeJsonString(_causeSentenceTheory->getFullParse()->getRoot()->toCasedTextString(_causeSentenceTheory->getTokenSequence())) << "\"\n";
	wss << L"        }";

	return wss.str();
}

std::wstring CauseEffectRelation::escapeJsonString(const std::wstring& input) {
    std::wstringstream ss;
    for (std::wstring::const_iterator iter = input.begin(); iter != input.end(); iter++) {
        switch (*iter) {
            case '\\': ss << "\\\\"; break;
            case '"': ss << "\\\""; break;
            case '/': ss << "\\/"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default: ss << *iter; break;
        }
    }
    return ss.str();
}
