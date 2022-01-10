// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "ProfileGenerator/ConditionHypothesis.h"
#include "ProfileGenerator/Profile.h"
#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"
#include "boost/foreach.hpp"
#include "boost/bind.hpp"
#include "boost/algorithm/string.hpp"

#include <iostream>
#include <wchar.h>

ConditionHypothesis::ConditionHypothesis(PGFact_ptr fact) {
	addFact(fact);
	PGFactArgument_ptr answerArg = fact->getAnswerArgument();
	if (answerArg)
		_value = answerArg->getLiteralStringValue();
	else _value = L"";
	
	// don't want to remove all "the"s, as they are a good signal that a condition is bad
	if (boost::iequals(_value, L"the flu"))
		_value = L"flu";

	if (boost::iequals(_value, L"pregnant"))
		_value = L"pregnancy";
	
	std::list<std::wstring> prefixesToRemove;
	prefixesToRemove.push_back(L"their ");
	prefixesToRemove.push_back(L"his ");
	prefixesToRemove.push_back(L"her ");
	prefixesToRemove.push_back(L"its ");
	prefixesToRemove.push_back(L"fewer ");
	prefixesToRemove.push_back(L"more ");	
	prefixesToRemove.push_back(L"advanced ");
	prefixesToRemove.push_back(L"popular ");
	prefixesToRemove.push_back(L"adjuvant ");
	prefixesToRemove.push_back(L"adjuvent ");
	prefixesToRemove.push_back(L"treat ");
	prefixesToRemove.push_back(L"control ");
	prefixesToRemove.push_back(L"treats ");
	prefixesToRemove.push_back(L"controls ");
	
	// repeat to make sure we get rid of all of them
	for (int i = 0; i < 2; i++) {
		BOOST_FOREACH(std::wstring prefix, prefixesToRemove) {
			if (boost::istarts_with(_value, prefix))
				_value = _value.substr(prefix.size());
		}
	}

	// eliminate things like "my mom's diabetes" but not "Alzheimer's disease"
	size_t possessive = _value.find(L"'s");
	if (possessive != std::wstring::npos && possessive + 3 < _value.size()) {
		std::wstring test_str = _value.substr(possessive+3);
		if (!boost::istarts_with(test_str, L"disease"))
			_value = test_str;
	}

	std::list<std::wstring> suffixesToRemove;
	suffixesToRemove.push_back(L",");
	suffixesToRemove.push_back(L"-LRB-");
	suffixesToRemove.push_back(L":");
	suffixesToRemove.push_back(L";");	
	suffixesToRemove.push_back(L" and ");	
	suffixesToRemove.push_back(L" who ");	
	suffixesToRemove.push_back(L" & ");	

	BOOST_FOREACH(std::wstring suffix, suffixesToRemove) {
		size_t index = _value.find(suffix);
		if (index != std::wstring::npos)
			_value = _value.substr(0, index);
	}

	boost::trim(_value);
	if (_value == L"")
		_value = L"NONE";

	_normalizedValue = _value;
	boost::to_lower(_normalizedValue);

	std::list<std::wstring> prefixesToRemoveForNormalization;
	prefixesToRemoveForNormalization.push_back(L"the ");
	prefixesToRemoveForNormalization.push_back(L"a ");
	prefixesToRemoveForNormalization.push_back(L"an ");
	prefixesToRemoveForNormalization.push_back(L"acute ");
	prefixesToRemoveForNormalization.push_back(L"high ");
	prefixesToRemoveForNormalization.push_back(L"serious ");
	prefixesToRemoveForNormalization.push_back(L"chronic ");
	prefixesToRemoveForNormalization.push_back(L"terminal ");

	BOOST_FOREACH(std::wstring prefix, prefixesToRemoveForNormalization) {
		if (boost::istarts_with(_normalizedValue, prefix))
			_normalizedValue = _normalizedValue.substr(prefix.size());
	}
}


std::wstring ConditionHypothesis::getDisplayValue() {
	if (_value == L"blood pressure" || _value == L"cholesterol") {
		// always high if unspecified
		return L"high " + _value;
	}
	return _value;
}

std::wstring ConditionHypothesis::getNormalizedDisplayValue() {
	return _normalizedValue;
}

bool ConditionHypothesis::isEquiv(GenericHypothesis_ptr hypoth) {
	ConditionHypothesis_ptr conditionHypoth = boost::dynamic_pointer_cast<ConditionHypothesis>(hypoth);
	if (conditionHypoth == ConditionHypothesis_ptr()) // cast failed
		return false;		

	std::wstring other_norm_value = conditionHypoth->getNormalizedDisplayValue();
	if (boost::iequals(_normalizedValue, other_norm_value))
		return true;

	if (boost::istarts_with(_normalizedValue, other_norm_value) || boost::istarts_with(other_norm_value, _normalizedValue)) {
		//std::wcout << L"STARTS WITH: " << _value << L" & " << other_norm_value << L"<br>\n";
		return true;
	}

	return false;
}

bool ConditionHypothesis::isSimilar(GenericHypothesis_ptr hypoth) {

	if (isEquiv(hypoth))
		return true;
	
	ConditionHypothesis_ptr conditionHypoth = boost::dynamic_pointer_cast<ConditionHypothesis>(hypoth);
	if (conditionHypoth == ConditionHypothesis_ptr()) // cast failed
		return false;		
	
	std::wstring other_norm_value = conditionHypoth->getNormalizedDisplayValue();

	if (boost::iends_with(_normalizedValue, other_norm_value) || boost::iends_with(other_norm_value, _normalizedValue))
		return true;

	if (boost::iends_with(_normalizedValue, L"stroke") && boost::iends_with(other_norm_value, L"stroke"))
		return true;

	return false;
}

void ConditionHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	
	ConditionHypothesis_ptr conditionHypo = boost::dynamic_pointer_cast<ConditionHypothesis>(hypo);
	if (conditionHypo == ConditionHypothesis_ptr())
		return;
	
	BOOST_FOREACH(PGFact_ptr fact, conditionHypo->getSupportingFacts()) {
		addFact(fact);
	}

	std::wstring other_value = conditionHypo->getDisplayValue();
	std::wstring other_norm_value = conditionHypo->getNormalizedDisplayValue();
	if (_value == L"cholesterol" &&
		(other_value == L"high cholesterol" || other_value == L"low cholesterol"))
	{
		_value = other_value; 
		_normalizedValue = other_norm_value; 		
	}
	if (boost::iequals(_normalizedValue, other_norm_value)) {
		//pass
	} else if (boost::istarts_with(_normalizedValue, other_norm_value)) {
		std::vector<std::wstring> thisWords;
		std::vector<std::wstring> otherWords;
		boost::split(thisWords, _normalizedValue, boost::is_any_of(L" "));
		boost::split(otherWords, other_norm_value, boost::is_any_of(L" "));
		if (thisWords.size() == otherWords.size()) {
			// switch to the shorter, probably a plural?
			_value = other_value; 
			_normalizedValue = other_norm_value; 			
		}
	} else if (boost::iends_with(other_norm_value, _normalizedValue)) {
		_value = other_value; 
		_normalizedValue = other_norm_value; 			
	}
}


int ConditionHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {

	// prefer specific forms of cancer
	if (getDisplayValue() == L"cancer" && boost::iends_with(hypo->getDisplayValue(), L"cancer"))
		return WORSE;
	else if (hypo->getDisplayValue() == L"cancer" && boost::iends_with(getDisplayValue(), L"cancer"))
		return BETTER;

	// Prefer the one with more facts
	if (nSupportingFacts() > hypo->nSupportingFacts())
		return BETTER;
	else if (hypo->nSupportingFacts() > nSupportingFacts())
		return WORSE;

	// Prefer the one with a better score group
	if (getBestScoreGroup() < hypo->getBestScoreGroup())
		return BETTER;
	else if (hypo->getBestScoreGroup() < getBestScoreGroup())
		return WORSE;

	// Break ties and show more recent epoch facts (with id as proxy) ahead of older epoch
	if (getNewestFactID() > hypo->getNewestFactID())
		return BETTER;
	else if (hypo->getNewestFactID() > getNewestFactID())
		return WORSE;

	// should never happen unless they share facts, which they shouldn't
	return SAME;

}

bool ConditionHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {

	if (boost::istarts_with(_value, L"the ")) {
		rationale = "starts with 'the '";
		return true;		
	}
	if (_value == L"disease") {
		rationale = "boring";
		return true;		
	}
	if (boost::istarts_with(_value, L"an ") || boost::istarts_with(_value, L"a ")) {
		rationale = "starts with 'an/a '";
		return true;		
	}
	if ((_value == L"death" || _value == L"deaths") && slot->getUniqueId() != "side_effect") {
		rationale = "death is not something that can be studied/treated";
		return true;		
	}

	if ((boost::istarts_with(_value, L"a ") || boost::istarts_with(_value, L"an ")) &&
		(boost::iends_with(_value, L"'s disease"))) 
	{
		rationale = "boring";
		return true;		
	}

	if (_value == L"NONE") {
		rationale = "empty";
		return true;		
	}
	return false;
}

bool ConditionHypothesis::isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale) {
	if (getBestScoreGroup() != 1)
		return true;

	return false;
}
