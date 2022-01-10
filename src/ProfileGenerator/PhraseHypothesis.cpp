// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/linuxPort/serif_port.h"

#include "ProfileGenerator/PhraseHypothesis.h"
#include "ProfileGenerator/Profile.h"
#include "Generic/common/OutputUtil.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/foreach.hpp"
#include "boost/bind.hpp"
#include "boost/algorithm/string.hpp"

#include <iostream>
#include <wchar.h>

PhraseHypothesis::PhraseHypothesis(PGFact_ptr fact) : _display_value(L"") {
	addFact(fact);
	setCoreFact(fact);

	std::wstring fact_string = L"";
	if (fact->getAnswerArgument())
		fact_string = fact->getAnswerArgument()->getLiteralStringValue();

	std::wstring phrase;
	std::locale loc;

	// Filter out all characters that are not alphanumeric or spaces
	// Use the non-resolved version of the string
	std::remove_copy_if(fact_string.begin(), fact_string.end(), std::back_inserter(phrase), 
		!(boost::bind(&std::isalnum<wchar_t>, _1, loc) ||
		  boost::bind(&std::isspace<wchar_t>, _1, loc)
		));	
	
	std::vector<std::wstring> words;
	boost::split(words, phrase, boost::is_space());
	BOOST_FOREACH(std::wstring word, words) {
		if (word.size() >= 4)
			_bigWords.insert(word);
	}
}

void PhraseHypothesis::setCoreFact(PGFact_ptr fact) {
	_coreFact = fact;
	_display_value = L""; // force it to recalculate
}

bool nonAscii(wchar_t wch) {
	return !iswascii(wch);
}

std::wstring PhraseHypothesis::getDisplayValue() {
	if (_display_value != L"")
		return _display_value;
	std::locale loc;

	std::wstring resolved = L"";
	if (_coreFact->getAnswerArgument())
		resolved = _coreFact->getAnswerArgument()->getResolvedStringValue();
	std::remove_copy_if(resolved.begin(), resolved.end(), std::back_inserter(_display_value), &nonAscii);
	_display_value = OutputUtil::untokenizeString(_display_value);	
	boost::replace_all(_display_value, "&apos;&apos;", "\"");
		
	return _display_value;
}

bool PhraseHypothesis::isEquiv(GenericHypothesis_ptr hypoth) {
	PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(hypoth);
	if (phraseHypoth == PhraseHypothesis_ptr()) // cast failed
		return false;		

	size_t n_total_big_words1 = _bigWords.size();
	size_t n_total_big_words2 = phraseHypoth->getBigWords().size();
	if (n_total_big_words1 < 1 && n_total_big_words2 > 1) return false;
	if (n_total_big_words2 < 1 && n_total_big_words1 > 1) return false;
	if (n_total_big_words1 < 6 || n_total_big_words2 < 6){
		// return a stricter measure of the strings -- they are too short for bag of words
		std::wstring ph1 = this->getDisplayValue();
		std::wstring ph2 = hypoth->getDisplayValue();
		if (ph1.size() == 0 && ph2.size() == 0) return true;
		if (ph1.size() == 0 || ph2.size() == 0) return false;
		return (boost::iequals(ph1.c_str(), ph2.c_str()));
	}

	int n_collisions = 0;
	BOOST_FOREACH (std::wstring word, phraseHypoth->getBigWords()) {
		if (_bigWords.find(word) != _bigWords.end())
			n_collisions++;
	}

	// If collided words make up 80% of either sentence's big words, then return true
	float thresh = .90f;
	return ((n_collisions > n_total_big_words1 * thresh) || (n_collisions > n_total_big_words2 * thresh));
}

bool PhraseHypothesis::isSimilar(GenericHypothesis_ptr hypoth) {
	PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(hypoth);
	if (phraseHypoth == PhraseHypothesis_ptr()) // cast failed
		return false;		

	size_t n_total_big_words1 = _bigWords.size();
	size_t n_total_big_words2 = phraseHypoth->getBigWords().size();
	int n_collisions = 0;
	BOOST_FOREACH (std::wstring word, phraseHypoth->getBigWords()) {
		if (_bigWords.find(word) != _bigWords.end())
			n_collisions++;
	}

	// If collided words make up 40% of either sentence's big words, then return true
	float thresh = .40f;
	return ((n_collisions > n_total_big_words1 * thresh) || (n_collisions > n_total_big_words2 * thresh));
}

void PhraseHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	
	PhraseHypothesis_ptr phraseHypo = boost::dynamic_pointer_cast<PhraseHypothesis>(hypo);
	if (phraseHypo == PhraseHypothesis_ptr())
		return;
	
	BOOST_FOREACH(PGFact_ptr fact, phraseHypo->getSupportingFacts()) {
		addFact(fact);
	}

	// For now, prefer longer sentences. 
	if (phraseHypo->getBigWords().size() > _bigWords.size()) {
		setCoreFact(phraseHypo->getCoreFact()); // this will also reset _display_value
		_bigWords.clear();
		BOOST_FOREACH(std::wstring word, phraseHypo->getBigWords()) {
			_bigWords.insert(word);
		}
	}

}

int PhraseHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {

	boost::gregorian::date_duration dd = getNewestCaptureTime() - hypo->getNewestCaptureTime();
	if (dd.days() > 20 || dd.days() < -20) {
		// Prefer the one that has the latest capture time
		if (getNewestCaptureTime() > hypo->getNewestCaptureTime())
			return BETTER;
		if (getNewestCaptureTime() < hypo->getNewestCaptureTime())
			return WORSE;
	}

	// Prefer hypothesis with a reliable English fact
	if (nReliableEnglishTextFacts() > 0 && hypo->nReliableEnglishTextFacts() == 0)
		return BETTER;
	if (hypo->nReliableEnglishTextFacts() > 0 && nReliableEnglishTextFacts() == 0)
		return WORSE;	
	
	// Prefer hypothesis with a reliable fact
	if (nReliableFacts() > 0 && hypo->nReliableFacts() == 0)
		return BETTER;
	if (hypo->nReliableFacts() > 0 && nReliableFacts() == 0)
		return WORSE;
	
	// Prefer hypothesis with any English fact
	if (nEnglishFacts() > 0 && hypo->nEnglishFacts() == 0)
		return BETTER;
	if (hypo->nEnglishFacts() > 0 && nEnglishFacts() == 0)
		return WORSE;

	// Prefer hypothesis from better score group
	if (getBestScoreGroup() < hypo->getBestScoreGroup())
		return BETTER;
	else if (hypo->getBestScoreGroup() < getBestScoreGroup())
		return WORSE;

	// Prefer the one that has the latest capture time
	if (getNewestCaptureTime() > hypo->getNewestCaptureTime())
		return BETTER;
	if (getNewestCaptureTime() < hypo->getNewestCaptureTime())
		return WORSE;

	// Break ties and show more recent epoch facts (with id as proxy) ahead of older epoch
	if (getNewestFactID() > hypo->getNewestFactID())
		return BETTER;
	else if (hypo->getNewestFactID() > getNewestFactID())
		return WORSE;

	// should never happen unless they share facts, which they shouldn't
	return SAME;

}

bool PhraseHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {
	if (_bigWords.size() < 6) {
		rationale = "TOO SHORT";
		return true;
	}
	std::wstring resolved = L"";
	if (_coreFact->getAnswerArgument())
		resolved = _coreFact->getAnswerArgument()->getResolvedStringValue();
	if (nEnglishFacts() == 0) {
		if (resolved.size() > 400) {
			rationale = "too long for foreign fact";
			return true;
		}
		// Kill coref unless it's just "He [name]" or "She [name]"
		size_t earliest_bracket = resolved.find(L"[");
		if (earliest_bracket != std::wstring::npos && earliest_bracket > 5) {
			rationale = "coref in foreign fact";
			return true;
		}
	}

	wchar_t last_char = resolved.at(resolved.size() - 1);
	if (last_char != L'.' && last_char != '\"' && last_char != '\'' && last_char != ';' && last_char != '!') {
		rationale = "phrase doesn't end in . or \" or ! or ;, might be truncated";
		return true;
	}

	PGFactArgument_ptr agentArg = _coreFact->getAgentArgument();
	if (agentArg != PGFactArgument_ptr() && agentArg->getEntityType() == "ORG" && !agentArg->isReliable()) {
		rationale = "non-reliable agent ORG mention";
		return true;
	}

	// If there is no TARGET_ENTITY in the resolved string val, our system didn't think
	//   we needed to resolve this reference, meaning the name is somewhere in the sentence
	// This is no longer valid in the actor-centric world; it'd be nice to replace someday... talk to Liz

	return false;		
}

bool PhraseHypothesis::isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale) {
	// We take care of foreign hypotheses explicitly in ProfileSlot
	return false;
}

bool PhraseHypothesis::isSpecialIllegalHypothesis(std::vector<PhraseHypothesis_ptr>& phrasesAlreadyDisplayed, 
												  std::string& rationale){
	BOOST_FOREACH(PhraseHypothesis_ptr hypo, phrasesAlreadyDisplayed) {
		if (isEquiv(hypo)){
			rationale = "intra-profile repeated phrase fact";	
			return true;
		}
	}
	return false;
}
bool PhraseHypothesis::isPartiallyTranslated(std::string& rationale){
	std::wstring phrase = getDisplayValue();
	if (phrase.find(L"()") != std::wstring::npos){
		rationale = "incomplete translation";
		return true;
	}
	return false;
}

//  ellipsis most likely came from a truncated sentence unless it is at the end.
bool PhraseHypothesis::isInternallyEllided(std::string& rationale){
	std::wstring phrase = getDisplayValue();
	size_t firstEllision = phrase.find(L"...");
	if ((firstEllision != std::wstring::npos) && (firstEllision != phrase.length()-3)){
		rationale = "ellision inside phrase";
		return true;
	}
	return false;
}

std::vector<PhraseHypothesis_ptr> PhraseHypothesis::existingProfilePhrases(Profile_ptr existing_profile){ 
	std::vector<PhraseHypothesis_ptr> phrasesAlreadyDisplayed;
	if (existing_profile != Profile_ptr()){
		std::pair<std::string, ProfileSlot_ptr> typeSlotPair;
		BOOST_FOREACH(typeSlotPair, existing_profile->getSlots()) {
			std::list<GenericHypothesis_ptr> outputHypotheses = typeSlotPair.second->getOutputHypotheses();
			BOOST_FOREACH(GenericHypothesis_ptr hypo, outputHypotheses) {
				PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(hypo);
				if (phraseHypoth != PhraseHypothesis_ptr()) {
					phrasesAlreadyDisplayed.push_back(phraseHypoth);
				}
			}
		}
	}
	return phrasesAlreadyDisplayed;
}
