//Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/UTF8InputStream.h"

#include "Generic/common/InputUtil.h"
#include "ProfileGenerator/NameHypothesis.h"
#include "ProfileGenerator/ProfileGenerator.h"

#include "Generic/common/ParamReader.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/algorithm/string/predicate.hpp"

#include <set>
#include <string>
#include <iostream>

std::set<std::wstring> NameHypothesis::_maleFirstNames;
std::set<std::wstring> NameHypothesis::_femaleFirstNames;
std::vector<std::wstring> NameHypothesis::_honorifics;
std::vector<std::wstring> NameHypothesis::_prefixHonorifics;
std::set<std::wstring> NameHypothesis::_lowercaseNameWords;

// each array must contain PRONOUN_ROLES_SIZE elements ordered to align with PrononRoles enum
const std::wstring NameHypothesis::pro_unknown[] = {L"", L"", L"", L""};
const std::wstring NameHypothesis::pro_inanimate[] = {L"it", L"it", L"its", L"its"};
const std::wstring NameHypothesis::pro_second[] = {L"you", L"you", L"your", L"yours"};
const std::wstring NameHypothesis::pro_male[] = {L"he", L"him", L"his", L"his"};
const std::wstring NameHypothesis::pro_female[] = {L"she", L"her", L"her", L"hers"};
const std::wstring NameHypothesis::pro_plural[] = {L"they", L"them", L"their", L"theirs"};

void NameHypothesis::initalizeStaticVectors() {
	static bool init = false;

	if (!init) {
		
		_lowercaseNameWords.insert(L"al");
		_lowercaseNameWords.insert(L"bin");
		_lowercaseNameWords.insert(L"bint");
		_lowercaseNameWords.insert(L"de");
		_lowercaseNameWords.insert(L"del");
		_lowercaseNameWords.insert(L"la");
		_lowercaseNameWords.insert(L"de la");

		_prefixHonorifics.push_back(L" king ");
		_prefixHonorifics.push_back(L" queen ");
		_prefixHonorifics.push_back(L" prince ");
		_prefixHonorifics.push_back(L" princess ");
		_prefixHonorifics.push_back(L" sir ");
		_prefixHonorifics.push_back(L" lord ");
		_prefixHonorifics.push_back(L" reverend ");
		_prefixHonorifics.push_back(L" father ");
		_prefixHonorifics.push_back(L" mother ");
		_prefixHonorifics.push_back(L" pastor ");
		_prefixHonorifics.push_back(L" pope ");
		_prefixHonorifics.push_back(L" mullah ");

		_honorifics.push_back(L" el sultan ");
		_honorifics.push_back(L" el sheikh ");
		_honorifics.push_back(L" the reverend ");
		_honorifics.push_back(L" sheikh ");
		_honorifics.push_back(L" sheikha ");
		_honorifics.push_back(L" grand ayatollah");
		_honorifics.push_back(L" ayatollah ");
		_honorifics.push_back(L" imam ");
		_honorifics.push_back(L" royal ");
		_honorifics.push_back(L" his highness ");
		_honorifics.push_back(L" her highness ");
		_honorifics.push_back(L" highness ");
		_honorifics.push_back(L" crown prince ");
		_honorifics.push_back(L" crown princess ");

		init = true;
	}
}

NameHypothesis::NameHypothesis(PGFact_ptr fact, ProfileSlot_ptr slot, PGDatabaseManager_ptr pgdm) : _pgdm(pgdm), _slot(slot)
{
	initalizeStaticVectors();
	addFact(fact);

	_is_person_slot = slot->isPerson();

	PGFactArgument_ptr answerArg = fact->getAnswerArgument();
	if (answerArg) {
		_actor_id = answerArg->getActorId();
		_displayValue = answerArg->getResolvedStringValue();
	} else {
		// This should never happen but very occasionally does when the fact pattern files let a non-named entity through
		SessionLogger::dbg("PG") << "Trying to make a NameHypothesis out of a fact without an answer argument; this is going to look very bad; fact ID = " << fact->getFactId();
		_actor_id = -1;
		_displayValue = L"";
	}

	initialize();
}

NameHypothesis::NameHypothesis(int actor_id, std::wstring canonical_name, bool is_person_slot, PGDatabaseManager_ptr pgdm) : 
_pgdm(pgdm), _slot(ProfileSlot_ptr()), _is_person_slot(is_person_slot)
{
	_actor_id = actor_id;
	_displayValue = canonical_name;
	initialize();
}

void NameHypothesis::initialize() {

	_actorStrings = _pgdm->getActorStringsForActor(_actor_id);
	for (ActorStringConfidenceMap::iterator iter = _actorStrings.begin(); iter != _actorStrings.end(); iter++) {
		addNameVariants(iter->first, iter->second);
	}
		
	_normalizedValue = ProfileGenerator::normalizeName(_displayValue);
	addNameString(_normalizedValue, 0.99);
	_display_value_up_to_date = false;
	_shortFormalName = _displayValue;

	std::wstring al_stripped_value = stripAlPrefix(_normalizedValue);
	if (_normalizedValue != al_stripped_value) {
		addNameString(_normalizedValue, 0.99);
	}

	if (isPersonSlot()){
		_normalizedValue = stripHonorifics(_normalizedValue);
		addNameString(_normalizedValue, 0.99);
		_gender = guessGenderFromFirstName(_normalizedValue);
	} else {
		_gender = INANIMATE;
	}

	fixDisplayCapitalization();
	_possibleDisplayValues[_displayValue]++;
	updateDisplayValue(); // this won't really do much, since there's only one thing in the map, but it's clean

	// If we haven't found a good short name for a two-name person, just 
	//   assume they are a normal American and use the last token as the short name
	if (_displayValue == _shortFormalName && isPersonSlot()) {
		std::vector<std::wstring> tokens;
		boost::split(tokens, _displayValue, boost::is_any_of(L" "));
		if (tokens.size() == 2)
			_shortFormalName = tokens.at(1);
	}
}


void NameHypothesis::addNameString(std::wstring name, double confidence) {
	if (_actorStrings.find(name) != _actorStrings.end()) {
		if (_actorStrings[name] < confidence)
			_actorStrings[name] = confidence;
	} else {
		_actorStrings[name] = confidence;
	}
}

void NameHypothesis::addNameVariants(std::wstring name, double confidence) {

	std::wstring normalized_name = ProfileGenerator::normalizeName(name);
	if (isPersonSlot())
		normalized_name = stripHonorifics(normalized_name);

	addNameString(normalized_name, confidence * 0.99);	
	
	std::wstring al_stripped_name = stripAlPrefix(normalized_name);
	if (normalized_name != al_stripped_name) {
		addNameString(al_stripped_name, confidence * 0.99);
	}
}

void NameHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {
	BOOST_FOREACH(PGFact_ptr fact, hypo->getSupportingFacts()) {
		addFact(fact);
	}
	NameHypothesis_ptr nameHypo = boost::dynamic_pointer_cast<NameHypothesis>(hypo);
	if (nameHypo == NameHypothesis_ptr())
		return;

	for (ActorStringConfidenceMap::iterator iter = nameHypo->getActorStrings().begin(); iter != nameHypo->getActorStrings().end(); iter++) {
		addNameString(iter->first, iter->second);
		addNameVariants(iter->first, iter->second);
	}

	typedef std::pair<std::wstring, int> wstr_int_t;
	BOOST_FOREACH(wstr_int_t other_pair, nameHypo->getPossibleDisplayValues()) {
		std::wstring otherNormalized = ProfileGenerator::normalizeName(other_pair.first);
		bool matched = false;
		BOOST_FOREACH(wstr_int_t our_pair, getPossibleDisplayValues()) {
			std::wstring ourNormalized = ProfileGenerator::normalizeName(our_pair.first);
			if (ourNormalized == otherNormalized) {
				_possibleDisplayValues[our_pair.first] += other_pair.second;
				matched = true;
			}
		}
		if (!matched)
			_possibleDisplayValues[other_pair.first] += other_pair.second;
	}
	_display_value_up_to_date = false;
}

NameHypothesis::ComparisonType NameHypothesis::compareValues(NameHypothesis_ptr otherNameHypothesis) {
	// This function is designed to be symmetric, except with regard to the difference between
	// BETTER and EQUAL.

	ActorStringConfidenceMap::iterator iter;
	
	if (getActorId() == otherNameHypothesis->getActorId())
		return EQUAL;

	// Have to use both because one might be the profile name which has no slot and therefore this can't be set
	bool is_family = (isFamily() || otherNameHypothesis->isFamily());

	// Use accessors to make sure everything is up to date
	std::wstring ourDisplayValue = getDisplayValue();
	std::wstring ourNormalizedValue = getNormalizedValue();
	std::wstring otherDisplayValue = otherNameHypothesis->getDisplayValue();
	std::wstring otherNormalizedValue = otherNameHypothesis->getNormalizedValue();

	if (ourDisplayValue == otherDisplayValue || ourNormalizedValue == otherNormalizedValue)
		return VERY_SIMILAR;

	if (ourDisplayValue.size() == 0 || otherDisplayValue.size() == 0)
		return NONE;

	double reliabilty_threshold = 0.5;

	// Must use normalized value here, since we are checking across xdoc	
	iter = _actorStrings.find(otherNormalizedValue);
	if (iter != _actorStrings.end() && iter->second > reliabilty_threshold)
		return VERY_SIMILAR;
		
	iter = otherNameHypothesis->getActorStrings().find(_normalizedValue);
	if (iter != otherNameHypothesis->getActorStrings().end() && iter->second > reliabilty_threshold)
		return VERY_SIMILAR;

	std::vector<std::wstring> ourTokens;
	std::vector<std::wstring> otherTokens;
	boost::split(ourTokens, ourNormalizedValue, boost::is_any_of(L" "));
	boost::split(otherTokens, otherNormalizedValue, boost::is_any_of(L" "));

	if (ourTokens.size() == 0 || otherTokens.size() == 0)
		return NONE; 

	std::wstring ourFirst = ourTokens.at(0);
	std::wstring otherFirst = otherTokens.at(0);
		
	if (isPersonSlot()){

		if (is_family) {
			// Only do this check for slots where we expect people to have the same last name (?)
			if (ourFirst == otherFirst) {
				// If we share first names, then the longer one is BETTER than the other
				if (ourTokens.size() == 1 && otherTokens.size() > 1)
					return BETTER;
				if (otherTokens.size() == 1 && ourTokens.size() > 1)
					return VERY_SIMILAR;
			}
		}

		std::wstring ourLast = ourTokens.at(ourTokens.size() - 1);
		std::wstring otherLast = otherTokens.at(otherTokens.size() - 1);

		// same first and last name
		if (ourFirst == otherFirst && ourLast == otherLast) {
			if (_normalizedValue.length() < otherNormalizedValue.length()){
				return BETTER;
			} else{
				return VERY_SIMILAR;
			}
		}

		// same last two names
		if (ourTokens.size() >= 2 && otherTokens.size() >= 2) {
			std::wstring ourLastTwo = getLastTwoNames(ourTokens);
			std::wstring otherLastTwo = getLastTwoNames(otherTokens);
			if (ourLastTwo == otherLastTwo) {
				if (_normalizedValue.length() < otherNormalizedValue.length()){
					return BETTER;
				} else{
					return VERY_SIMILAR;
				}
			}
		}

		if (ourLast == otherLast && ourFirst.size() > 3 && otherFirst.size() > 3 && ourFirst.substr(0,3) == otherFirst.substr(0,3))
			return SIMILAR;

		// If one of these is a single name only, and it agrees
		//  with the first name in its first letter and last letter, kill it!
		if (is_family) {
			if (ourTokens.size() == 1 || otherTokens.size() == 1) {
				if (ourFirst.at(0) == otherFirst.at(0) &&
					ourFirst.at(ourFirst.size() - 1) == otherFirst.at(otherFirst.size() - 1))
				{
					std::wcerr << L"Squashing " << ourFirst << L" " << otherFirst << L"\n";
					return SIMILAR;
				}
			}
		}
	}

	// Look at dash-squeezed names. These are different from normalized names, as they replace
	//  dashes with nothing rather than whitespace
	if (otherDisplayValue.find(L'-') != std::wstring::npos){
		std::wstring our_dash_squeezed_name = UnicodeUtil::dashSqueezedNormalizedName(ourDisplayValue);
		std::wstring other_dash_squeezed_name = UnicodeUtil::dashSqueezedNormalizedName(otherDisplayValue);
		if (our_dash_squeezed_name == other_dash_squeezed_name)
			return VERY_SIMILAR;

		iter = _actorStrings.find(other_dash_squeezed_name);
		if (iter != _actorStrings.end() && iter->second > reliabilty_threshold)
			return VERY_SIMILAR;

		iter = otherNameHypothesis->getActorStrings().find(our_dash_squeezed_name);
		if (iter != otherNameHypothesis->getActorStrings().end() && iter->second > reliabilty_threshold)
			return VERY_SIMILAR;
	}

	iter = _actorStrings.find(otherNormalizedValue);
	if (iter != _actorStrings.end())
		return SIMILAR;

	iter = otherNameHypothesis->getActorStrings().find(_normalizedValue);
	if (iter != otherNameHypothesis->getActorStrings().end() && iter->second > reliabilty_threshold)
		return SIMILAR;

    if (otherTokens.size() > 1 || ourTokens.size() > 1) {
		if (UnicodeUtil::nonTrivialSubName(_normalizedValue, otherNormalizedValue) ||
			UnicodeUtil::nonTrivialSubName(otherNormalizedValue, _normalizedValue))
		{
			return SIMILAR;
		}
	}

	return NONE;

}

std::wstring NameHypothesis::getLastTwoNames(std::vector<std::wstring>& tokens) const {
	if (tokens.size() == 0)
		return L"";
	std::wstring ret_value = tokens.back();
	// use int here and not size_t since we will go below 0
	for (int i = static_cast<int>(tokens.size()) - 2; i >= 0; i--) {
		ret_value = L" " + ret_value;
		ret_value = tokens.at(i) + ret_value;
		if (_lowercaseNameWords.find(tokens.at(i)) == _lowercaseNameWords.end())
			break;
	}
	return ret_value;
}

int NameHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {
	
	if (hasExternalHighConfidenceFact() && !hypo->hasExternalHighConfidenceFact())
		return BETTER;
	else if (hypo->hasExternalHighConfidenceFact() && !hasExternalHighConfidenceFact())
		return WORSE;

	// Prefer the one with the most double-reliable English facts
	if (nDoublyReliableEnglishTextFacts() > hypo->nDoublyReliableEnglishTextFacts())
		return BETTER;
	if (hypo->nDoublyReliableEnglishTextFacts() > nDoublyReliableEnglishTextFacts())
		return WORSE;
	
	// Prefer the one with the most double-reliable facts
	if (nDoublyReliableFacts() > hypo->nDoublyReliableFacts())
		return BETTER;
	if (hypo->nDoublyReliableFacts() > nDoublyReliableFacts())
		return WORSE;
	
	// Prefer the one with the most English facts
	if (nEnglishFacts() > hypo->nEnglishFacts())
		return BETTER;
	if (hypo->nEnglishFacts() > nEnglishFacts())
		return WORSE;

	// Prefer the one with the most facts overall
	if (nSupportingFacts() > hypo->nSupportingFacts())
		return BETTER;
	if (hypo->nSupportingFacts() > nSupportingFacts())
		return WORSE;

	// IDEA: something about actual descriptor quality, though perhaps this belongs in FF

	// Break ties and show more recent epoch facts (with id as proxy) ahead of older epoch
	if (getNewestFactID() > hypo->getNewestFactID())
		return BETTER;
	else if (hypo->getNewestFactID() > getNewestFactID())
		return WORSE;

	// should never happen unless they share facts, which they shouldn't
	return SAME;

}

bool NameHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {

	if (slot->getUniqueId() == ProfileSlot::VISIT_UNIQUE_ID && nDoublyReliableFacts() < 5 && getBestDoublyReliableScoreGroup() != 1) {
		rationale = "visit; fewer than five doubly-reliable facts and no doubly-reliable top score group";
		return true;
	}

	std::wstring lowDispVal = getDisplayValue();
	if (L"NO_NAME" == lowDispVal || L"No_name" == lowDispVal || L"" == lowDispVal){
		rationale = "missing name";
		return true;
	}
	std::transform(lowDispVal.begin(), lowDispVal.end(), lowDispVal.begin(), towlower);
	if (L"al" == lowDispVal || L"de" == lowDispVal || L"bin" == lowDispVal){
		rationale = "name is empty linker word";
		return true;
	}

	// could theoretically allow these later but for now let's not
	if (L"muslim" == lowDispVal || L"muslims" == lowDispVal || 
		L"arab" == lowDispVal || L"arabs" == lowDispVal || 
		L"jewish" == lowDispVal || L"jew" == lowDispVal || L"jews" == lowDispVal || 
		L"christian" == lowDispVal || L"christians" == lowDispVal || 
		L"sunni" == lowDispVal || L"sunnis" == lowDispVal || 
		L"shi'ite" == lowDispVal || L"shi'ites" == lowDispVal || L"shi'a" == lowDispVal || 
		L"shiite" == lowDispVal || L"shiites" == lowDispVal || L"shia" == lowDispVal || 
		L"kurd" == lowDispVal || L"kurds" == lowDispVal || 
		L"islamist" == lowDispVal || L"islamists" == lowDispVal || 
		L"islamic" == lowDispVal) 
	{
		rationale = "name is person group";
		return true;
	}   
	
	// boring locations
	if (L"north" == lowDispVal || L"northern" == lowDispVal || 
		L"south" == lowDispVal || L"southern" == lowDispVal || 
		L"east" == lowDispVal || L"eastern" == lowDispVal || 
		L"west" == lowDispVal || L"western" == lowDispVal ||
		L"southeast" == lowDispVal || L"southeastern" == lowDispVal || 
		L"southwest" == lowDispVal || L"southwestern" == lowDispVal ||
		L"south-east" == lowDispVal || L"south-eastern" == lowDispVal || 
		L"south-west" == lowDispVal || L"south-western" == lowDispVal ||		
		L"northeast" == lowDispVal || L"northeastern" == lowDispVal || 
		L"northwest" == lowDispVal || L"northwestern" == lowDispVal ||
		L"north-east" == lowDispVal || L"north-eastern" == lowDispVal || 
		L"north-west" == lowDispVal || L"north-western" == lowDispVal)
	{
		rationale = "name is location modifier";
		return true;
	}   

	if (slot->isPerson() && !slot->isFamily()) {
		// Disallow single-word names for slots that should be non-family people
		if (getDisplayValue().find(L" ") == std::wstring::npos) {
			rationale = "single-word non-family name";
			return true;
		}
		if (L"identity" == lowDispVal || L"identity card" == lowDispVal){
			rationale = "known bug associates identity card";
			return true;
		}
	}
	if (lowDispVal == L"the academy" || lowDispVal == L"the university" || lowDispVal == L"the school" || lowDispVal == L"the centre" ||
		lowDispVal == L"academy" || lowDispVal == L"university" || lowDispVal == L"school" || lowDispVal == L"centre")
	{
		rationale = "generic name; probably bad translation";
		return true;
	}
	// We only trust reliable answers and agents
	if (nDoublyReliableFacts() == 0) {
		rationale = "not doubly-reliable";
		return true;
	}
	return false;
}	

bool NameHypothesis::isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale) { 
	if (nEnglishFacts() == 0 && nDoublyReliableFacts() <= 1) {
		rationale = "non-English, one or zero doubly-reliable fact";
		return true;
	}
	if (slot->getUniqueId() == ProfileSlot::VISIT_UNIQUE_ID && nDoublyReliableFacts() <= 1) {
		rationale = "visit; one or zero doubly-reliable fact";
		return true;
	}
	return false;
}


bool NameHypothesis::matchesProfileName(Profile_ptr profile) {
	if (matchesName(profile->getName()))
		return true;
	if (profile->getProfileType() == Profile::PER && !isPersonSlot())
		return false;
	if (profile->getProfileType() != Profile::PER && isPersonSlot())
		return false;
	NameHypothesis_ptr profNameHyp = profile->getProfileNameHypothesis();
	if (isSimilar(profNameHyp) || profNameHyp->isSimilar(shared_from_this()))
		return true;

	return false;
}

bool NameHypothesis::matchesName(std::wstring test_name) {

	if (test_name == getDisplayValue() || test_name == getNormalizedValue())
		return true;

	std::wstring norm_test_name = ProfileGenerator::normalizeName(test_name);
	std::wstring stripped = stripHonorifics(norm_test_name);
	return (norm_test_name == _normalizedValue || stripped == _normalizedValue || 
		_actorStrings.find(norm_test_name) != _actorStrings.end() || 
		_actorStrings.find(stripped) != _actorStrings.end());
}

std::wstring NameHypothesis::stripAlPrefix(std::wstring name){
	// assumes name has been normalized already
	std::wstring new_name = name;
	boost::replace_all(new_name, L" al", L"");
	if (boost::istarts_with(new_name, L"al "))
		new_name = new_name.substr(3);	
	if (new_name != L"")
		return new_name;
	else return name;
}
std::wstring NameHypothesis::stripHonorifics(std::wstring name){
	std::wstring stripped = L" " + name + L" ";
	bool changed = false;
	BOOST_FOREACH(std::wstring honorific, _honorifics) {
		size_t start = stripped.find(honorific);
		if (start != std::string::npos) {
			changed = true;
			stripped.replace(start, honorific.size(), L" ");
		}
	}
	BOOST_FOREACH(std::wstring honorific, _prefixHonorifics) {
		size_t start = stripped.find(honorific);
		if (start == 0) { // the match must be at the beginning of the string
			changed = true;
			stripped.replace(start, honorific.size(), L" ");
		}
	}
	std::wstring candidate = stripped.substr(1,stripped.length()-2);
	if (changed && candidate != L"")
		return candidate;
	else
		return name;
}
std::wstring NameHypothesis::getPronounString(PronounRole role, GenderType gender_type){
	switch(gender_type) {
		case UNKNOWN: 
			return pro_unknown[role];
		case INANIMATE:
			return pro_inanimate[role];
		case SECOND:
			return pro_second[role];
		case MALE: 
			return pro_male[role]; 
		case FEMALE:
			return pro_female[role];
		case PLURAL_ORG: 
			return pro_plural[role];
		default: 
			return pro_unknown[role];
	}
}

NameHypothesis::GenderType NameHypothesis::guessGenderFromFirstName(std::wstring normedPersonName){
	std::wstring firstName = normedPersonName;
	size_t space = normedPersonName.find(L' ');
	if (space != std::wstring::npos)
		firstName = normedPersonName.substr(0, space);
	std::transform(firstName.begin(), firstName.end(), firstName.begin(), towlower);

	if (_maleFirstNames.find(firstName) != _maleFirstNames.end())
		return MALE;
	if (_femaleFirstNames.find(firstName) != _femaleFirstNames.end())
		return FEMALE;
	return UNKNOWN;
}

void NameHypothesis::setPersonShortFormalName(std::wstring candidate) {

	// The full name must end with the candidate value, ignoring case
	// I don't normalize for spaces and such here, that seems like too much trouble
	if (!boost::iends_with(_displayValue, L" " + candidate))
		return;

	for (std::set<std::wstring>::iterator iter = _lowercaseNameWords.begin(); iter != _lowercaseNameWords.end(); iter++) {
		std::wstring lc_word = *iter;
		std::wstring test_candidate = lc_word + L" " + candidate;
		if (boost::iends_with(_displayValue, L" " + test_candidate)) {
			candidate = test_candidate;
			break;
		}
	}

	//if (_shortFormalName != _displayValue)
//		std::wcerr << "Replacing short formal name " << _shortFormalName << L" with " << candidate << L"\n";
//	else std::wcerr << _displayValue << L" --> " << candidate << L"\n";

	_shortFormalName = candidate;	

}
void NameHypothesis::loadGenderNameLists() {
	static bool init = false;
	if (!init) {
		_maleFirstNames = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("male_names_list"), false, true);
		_femaleFirstNames = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("female_names_list"), false, true);
		init = true;
	}	
}

std::wstring NameHypothesis::getSimpleCapitalization(std::wstring phrase) {
	// Upcase every word except for "of" and "in"
	std::wstring cap = L"";
	for (size_t i = 0; i < phrase.size(); i++) {
		if (i == 0 || phrase.at(i-1) == L' ' || phrase.at(i-1) == L'-')
			cap += towupper(phrase.at(i));
		else cap += towlower(phrase.at(i));
	}
	boost::algorithm::replace_all(phrase, L" Of ", L" of ");
	boost::algorithm::replace_all(phrase, L" In ", L" in ");	
	return cap;
}

std::wstring NameHypothesis::fixLowercaseNameWords(std::wstring phrase, bool is_short_name) {
	// Change lowercase-name-words to lowercase (e.g. 'al', 'del')
	BOOST_FOREACH(std::wstring lc_word, _lowercaseNameWords) {
		std::wstring correct = lc_word + L" ";

		// If this is a full name, only do the correction in the middle of the name
		if (!is_short_name)
			correct = L" " + correct;

		std::wstring incorrect = getSimpleCapitalization(correct);	
		boost::algorithm::replace_all(phrase, incorrect, correct);
	}
	return phrase;
}

void NameHypothesis::fixDisplayCapitalization() {

	// Fix capitalization for _displayValue and _shortFormalName
	if (_displayValue == L"Palestinian") {
		_displayValue = L"Palestine";
		_shortFormalName = L"Palestine";
		return;
	}

	bool long_has_upper = false;
	bool long_has_lower = false;
	for (size_t i = 0; i < _displayValue.size(); i++) {
		if (iswupper(_displayValue.at(i)))
			long_has_upper = true;
		else if (iswlower(_displayValue.at(i)))
			long_has_lower = true;
		if (long_has_upper && long_has_lower)
			break;
	}

	bool short_has_upper = false;
	bool short_has_lower = false;
	for (size_t i = 0; i < _shortFormalName.size(); i++) {
		if (iswupper(_shortFormalName.at(i)))
			short_has_upper = true;
		else if (iswlower(_shortFormalName.at(i)))
			short_has_lower = true;
		if (short_has_upper && short_has_lower)
			break;
	}

	// If we're already mixed case, don't do anything to _displayValue
	if (long_has_upper && long_has_lower) {

		// Fix Bin Laden --> bin Laden
		if (_displayValue != _shortFormalName && isPersonSlot()) {
			_shortFormalName = fixLowercaseNameWords(_shortFormalName, true);	
		}

		if (_shortFormalName != _displayValue) {
			if (!short_has_upper || (isPersonSlot() && !short_has_lower)) {
				std::wstring newShortFormalName = getSimpleCapitalization(_shortFormalName);
				newShortFormalName = fixLowercaseNameWords(newShortFormalName, true);	
				_shortFormalName = newShortFormalName;
			}
		}

		return;
	}

	if (_slot != ProfileSlot_ptr() && _slot->isLocation()) {
		// Change things like NEW YORK --> New York
		_displayValue = getSimpleCapitalization(_displayValue);
		_shortFormalName = _displayValue;
		return;
	}

	// Too dangerous to deal with all-uppercase ORGs, since they can be acronyms
	if (!isPersonSlot()){
		if (!long_has_upper) { 
			_displayValue = getSimpleCapitalization(_displayValue);
			_shortFormalName = _displayValue;
		}
		return;
	}

	std::wstring newDisplayValue = getSimpleCapitalization(_displayValue);
	newDisplayValue = fixLowercaseNameWords(newDisplayValue, false);	
	
	if (_displayValue == _shortFormalName) {
		_shortFormalName = newDisplayValue;
	} else {
		std::wstring newShortFormalName = getSimpleCapitalization(_shortFormalName);
		newShortFormalName = fixLowercaseNameWords(newShortFormalName, true);	
		_shortFormalName = newShortFormalName;
	}

	_displayValue = newDisplayValue;		
}

// returns full name (_displayValue) for 1st, _shortFormalName for second, and
// pronoun that matches the syntactic role for all greater mentions.
std::wstring NameHypothesis::getRepeatReference(int& counter, PronounRole role){
	counter++;
	if (counter == 1) {
		if (role == POSSESSIVE_MOD || role == POSSESSIVE)
			return _displayValue + L"'s";
		else return _displayValue;
	}
	if (counter == 2 && _displayValue != _shortFormalName) {
		if (role == POSSESSIVE_MOD || role == POSSESSIVE)
			return _shortFormalName + L"'s";
		else return _shortFormalName;
	}
	std::wstring pro = getPronounString(role, _gender);
	if (pro != L"")
		return pro;
	if (role == POSSESSIVE_MOD || role == POSSESSIVE)
		return _shortFormalName + L"'s";
	else return _shortFormalName;
}


std::wstring NameHypothesis::getDisplayValue() {
	if (!_display_value_up_to_date)
		updateDisplayValue();
	return _displayValue;
}

void NameHypothesis::updateDisplayValue() {
	typedef std::pair<std::wstring, int> wstr_int_pair_t;	

	if (isPersonSlot()) {

		std::wstring longest_display_value = L"";
		int total_display_values = 0;
		bool exists_non_singleton = false;
		BOOST_FOREACH(wstr_int_pair_t my_pair, _possibleDisplayValues) {
			if (my_pair.first.find(L" ") == std::wstring::npos)
				continue;
			total_display_values += my_pair.second;
			if (my_pair.second > 1)
				exists_non_singleton = true;
			//std::wcout << L"  * " << my_pair.first << L": " << my_pair.second << L"\n";
		}

		BOOST_FOREACH(wstr_int_pair_t my_pair, _possibleDisplayValues) {
			if (my_pair.first.size() > longest_display_value.size()) {
				if (total_display_values >= 5 && my_pair.second < 2) {
					//std::wcout << L"Killing possibly fluke name: " << my_pair.first << L"\n";
					continue;
				} else if (exists_non_singleton && my_pair.second == 1) {
					//std::wcout << L"Killing singleton name: " << my_pair.first << L"\n";
					continue;
				}
				longest_display_value = my_pair.first;
			}
		}

		_displayValue = longest_display_value;

	} else {

		int max_count = 0;
		std::wstring most_frequent_display_value = L"";
		BOOST_FOREACH(wstr_int_pair_t my_pair, _possibleDisplayValues) {
			if (my_pair.second > max_count) {
				max_count = my_pair.second;
				most_frequent_display_value = my_pair.first;
			}
		}

		_displayValue = most_frequent_display_value;

	}

	//std::wcout << L"RESULT: " << _displayValue << L"\n";

	_normalizedValue = ProfileGenerator::normalizeName(_displayValue);
	fixDisplayCapitalization();	
	_display_value_up_to_date = true;
}

std::vector<GenericHypothesis::kb_arg_t> NameHypothesis::getKBArguments(int actor_id, ProfileSlot_ptr slot) {
	std::vector<kb_arg_t> kb_args;

	kb_arg_t focus;
	focus.actor_id = actor_id;
	focus.value = L"";
	focus.role = slot->getFocusRole();
	kb_args.push_back(focus);

	kb_arg_t answer;
	answer.actor_id = getActorId();
	answer.value = L"";
	answer.role = slot->getAnswerRole();
	kb_args.push_back(answer);

	return kb_args;
}
