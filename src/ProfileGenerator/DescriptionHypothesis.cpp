// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InputUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"

#include "ProfileGenerator/DescriptionHypothesis.h"
#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/PGFact.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"
#include <string>
#include <iostream>
#include <sstream>

std::set<std::wstring> DescriptionHypothesis::_maleDescriptions;
std::set<std::wstring> DescriptionHypothesis::_femaleDescriptions;

DescriptionHypothesis::DescriptionHypothesis(PGFact_ptr fact, PGDatabaseManager_ptr pgdm) : _headword(L""), _display_value(L"")
{
	_is_title = false;
	_is_copula = false;
	_is_appositive = false;

	setCoreFact(fact);
	addFact(fact);
	updateCountsForNewFact(fact);

	// should only be one
	PGFactArgument_ptr headwordArgument = fact->getFirstArgumentForRole("Headword");
	if (headwordArgument) {
		_headword = headwordArgument->getLiteralStringValue();
		std::transform(_headword.begin(), _headword.end(), _headword.begin(), towlower);
	} 

	std::vector<PGFactArgument_ptr> mentionArgFacts = fact->getArgumentsForRole("MentionArg");
	BOOST_FOREACH(PGFactArgument_ptr arg, mentionArgFacts) {
		_mentionArguments.push_back(boost::make_shared<NameHypothesis>(arg->getActorId(), arg->getResolvedStringValue(), false, pgdm));
	}

	std::vector<PGFactArgument_ptr> nonMentionArgFacts = fact->getArgumentsForRole("NonMentionArg");
	BOOST_FOREACH(PGFactArgument_ptr arg, nonMentionArgFacts) {
		_nonMentionArguments.push_back(boost::make_shared<SimpleStringHypothesis>(fact, arg));
	}

	// 30 is good default
	_max_equivalence_distance = ParamReader::getRequiredIntParam("desc_hyp_max_distance");
}

void DescriptionHypothesis::setCoreFact(PGFact_ptr fact) {
	_coreFact = fact;
	_display_value = L""; // force it to recalculate
}


void DescriptionHypothesis::loadGenderDescLists() {
	static bool init = false;
	if (!init) {
		_maleDescriptions = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("male_descriptors_list"), false, true);
		_femaleDescriptions = InputUtil::readFileIntoSet(ParamReader::getRequiredParam("female_descriptors_list"), false, true);
		init = true;
	}	
}

void DescriptionHypothesis::updateCountsForNewFact(PGFact_ptr fact) {
	PGFactArgument_ptr agentArg = fact->getAgentArgument();
	if (!agentArg)
		return;

	if (agentArg->getSERIFMentionConfidence() == MentionConfidenceStatus::COPULA_DESC)
		_is_copula = true;
	if (agentArg->getSERIFMentionConfidence() == MentionConfidenceStatus::TITLE_DESC)
		_is_title = true;
	if (agentArg->getSERIFMentionConfidence() == MentionConfidenceStatus::APPOS_DESC)
		_is_appositive = true;

	if (fact->isSerif() && agentArg->getSERIFMentionConfidence() == MentionConfidenceStatus::UNKNOWN_CONFIDENCE) {
		if (agentArg->getConfidence() > 0.829 && agentArg->getConfidence() < 0.831)
			_is_copula = true;
		if (agentArg->getConfidence() > 0.819 && agentArg->getConfidence() < 0.821)
			_is_appositive = true;
		if (agentArg->getConfidence() > 0.809 && agentArg->getConfidence() < 0.811)
			_is_title = true;
	}
}
		

void DescriptionHypothesis::addSupportingHypothesis(GenericHypothesis_ptr hypo) {

	// VERY IMPORTANT:
	// We assume that this hypothesis either subsumes or is subsumed by the hypothesis
	// to which it is being added.

	// Only add other DescriptionHypothesis_ptrs
	DescriptionHypothesis_ptr descHypo = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypo);
	if (descHypo == DescriptionHypothesis_ptr())
		return;

	BOOST_FOREACH(PGFact_ptr fact, hypo->getSupportingFacts()) {
		addFact(fact);
		updateCountsForNewFact(fact);
	}
	
	int n_modifiers = getNModifiers();
	int other_n_modifiers = descHypo->getNModifiers();

	if (n_modifiers < other_n_modifiers) {
		// then really we want these things to switch places, but we're going to be
		// a bit hacky about it
		setCoreFact(descHypo->getCoreFact());
		_mentionArguments.clear();
		_nonMentionArguments.clear();
		BOOST_FOREACH(NameHypothesis_ptr h, descHypo->getMentionArguments()) {
			_mentionArguments.push_back(h);
		}
		BOOST_FOREACH(SimpleStringHypothesis_ptr h, descHypo->getNonMentionArguments()) {
			_nonMentionArguments.push_back(h);
		}
	}
}

bool DescriptionHypothesis::isEquiv(GenericHypothesis_ptr hypoth) { 	
	DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypoth);
	if (descHypoth == DescriptionHypothesis_ptr())
		return false;

	if (!isEquivHeadword(descHypoth->getHeadword())) {
		return false;
	}


	std::wstring display_value = getDisplayValue();
	std::wstring other_display_value = hypoth->getDisplayValue();
	if (display_value == other_display_value)
		return true;

	// if very different in size, they can't be equal
	int diff = abs(static_cast<int>(display_value.size()) - static_cast<int>(other_display_value.size()));
	if (diff > _max_equivalence_distance)
		return false;

	if (getNModifiers() > 0 && descHypoth->getNModifiers() > 0) {
		if (isSubsumedBy(descHypoth) && descHypoth->isSubsumedBy(shared_from_this()))
			return true;
		else return false;
	} else if (getNModifiers() > 0 || descHypoth->getNModifiers() > 0) {
		return false;
	} else return true;
}
bool DescriptionHypothesis::isSimilar(GenericHypothesis_ptr hypoth) { 
	DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypoth);
	if (descHypoth == DescriptionHypothesis_ptr())
		return false;
	
	if (isGenericORGHeadword(_headword) || isGenericPERHeadword(_headword)) {
		// allow headword matches
		return hasOverlappingModifiers(descHypoth);
	}

	return (isEquivHeadword(descHypoth->getHeadword()) || hasOverlappingModifiers(descHypoth));
}

bool DescriptionHypothesis::isEquivHeadword(std::wstring otherHeadword) { 
	if (_headword == otherHeadword)
		return true;

	if (isGenericORGHeadword(_headword) && isGenericORGHeadword(otherHeadword))
		return true;

	if (isGenericPERHeadword(_headword) && isGenericPERHeadword(otherHeadword))
		return true;

	return false;
}

bool DescriptionHypothesis::isGenericORGHeadword(std::wstring headword) {
	return (headword == L"group" || headword == L"organization" ||
			headword == L"party" || headword == L"company" ||
			headword == L"faction" || headword == L"sect");
}
bool DescriptionHypothesis::isGenericPERHeadword(std::wstring headword) {
	return (headword == L"leader" || headword == L"man" || headword == L"woman" ||
			headword == L"person" ||
			headword == L"chief" || headword == L"head");
}

bool DescriptionHypothesis::isSubsumedBy(DescriptionHypothesis_ptr otherDesc) { 
	// All of our modifiers must be covered by otherDesc
	for (std::vector<NameHypothesis_ptr>::iterator iter = _mentionArguments.begin(); iter != _mentionArguments.end(); iter++) {
		if (!otherDesc->hasMentionModifier(*iter))
			return false;
	}
	for (std::vector<SimpleStringHypothesis_ptr>::iterator iter = _nonMentionArguments.begin(); iter != _nonMentionArguments.end(); iter++) {
		if (!otherDesc->hasNonMentionModifier(*iter))
			return false;
	}
	return true;
}

bool DescriptionHypothesis::hasOverlappingModifiers(DescriptionHypothesis_ptr otherDesc) { 
	for (std::vector<NameHypothesis_ptr>::iterator iter = _mentionArguments.begin(); iter != _mentionArguments.end(); iter++) {
		if (otherDesc->hasMentionModifier(*iter))
			return true;
	}
	for (std::vector<SimpleStringHypothesis_ptr>::iterator iter = _nonMentionArguments.begin(); iter != _nonMentionArguments.end(); iter++) {
		if (otherDesc->hasNonMentionModifier(*iter))
			return true;
	}
	return false;
}

bool DescriptionHypothesis::hasMentionModifier(NameHypothesis_ptr hypo) {
	for (std::vector<NameHypothesis_ptr>::iterator iter = _mentionArguments.begin(); iter != _mentionArguments.end(); iter++) {
		if (hypo->isEquiv(*iter))
			return true;
	}
	return false;
}
bool DescriptionHypothesis::hasNonMentionModifier(SimpleStringHypothesis_ptr hypo) {	
	for (std::vector<SimpleStringHypothesis_ptr>::iterator iter = _nonMentionArguments.begin(); iter != _nonMentionArguments.end(); iter++) {
		if (hypo->isEquiv(*iter))
			return true;
	}
	return false;
}

std::wstring DescriptionHypothesis::getDisplayValue() { 
	// Caching; we only calculate this once
	if (_display_value != L"")
		return _display_value;

	PGFactArgument_ptr descExtentArg = _coreFact->getFirstArgumentForRole("Extent");
	if (!descExtentArg)
		return L"";

	_display_value = descExtentArg->getResolvedStringValue();

	// IDEA: replace with most common description?
	
	// for copulas that start with n't
	if (_display_value.find(L"n't") == 0)
		boost::replace_first(_display_value, L"n't", L"not");		

	// remove quotation marks in descriptions
	boost::replace_all(_display_value,L"''",L"");
	boost::replace_all(_display_value,L"``",L"");
	boost::replace_all(_display_value, "&apos;&apos;", "");
	boost::replace_all(_display_value, "\"", "");

	boost::replace_all(_display_value,L"  ",L" ");
	
	boost::replace_all(_display_value,L"The ",L"the "); 
	boost::replace_all(_display_value,L"That ",L"that "); 
	boost::replace_all(_display_value,L"This ",L"this "); 
	boost::replace_all(_display_value,L"A ",L"a "); 
	boost::replace_all(_display_value,L"An ",L"an "); 

	// Remove mismatching parens
	if (_display_value.find(L")") != std::wstring::npos && _display_value.find(L"(") == std::wstring::npos) {
		boost::replace_all(_display_value, L")", L"");
	}	
	if (_display_value.find(L"(") != std::wstring::npos && _display_value.find(L")") == std::wstring::npos) {
		boost::replace_all(_display_value, L"(", L"");
	}

	_display_value = OutputUtil::untokenizeString(_display_value);	
		
	return _display_value;
}	

void DescriptionHypothesis::dump_to_screen() {
	SessionLogger::info("PG") << "Description Hypothesis:\n";
	SessionLogger::info("PG") << "Fact ID: " << _coreFact->getFactId() << "\n";
	SessionLogger::info("PG") << L"Headword: " << _headword << L"\n";

	BOOST_FOREACH (NameHypothesis_ptr arg, _mentionArguments) {
		SessionLogger::info("PG") << L"Mention: " << arg->getDisplayValue() << L"\n";
	}
	BOOST_FOREACH (SimpleStringHypothesis_ptr arg, _nonMentionArguments) {
		SessionLogger::info("PG") << L"String: " << arg->getDisplayValue() << L"\n";
	}
}


int DescriptionHypothesis::rankAgainst(GenericHypothesis_ptr hypo) {

	// All of these should be "reliable"; we don't upload descriptors that come from pure coref
	// They are also all the same score group

	// Allow English facts to determine reliability if they are sufficiently represented
	/*bool english_is_valid_criterion = false;
	if (nEnglishFacts() > 5 || hypo->nEnglishFacts() > 5 ||
		(float)nEnglishFacts() / nSupportingFacts() > 0.3F ||
		(float)hypo->nEnglishFacts() / hypo->nSupportingFacts() > 0.3F)
		english_is_valid_criterion = true;

	if (english_is_valid_criterion) {
		// Prefer the one with the most English facts
		if (nEnglishFacts() > hypo->nEnglishFacts())
			return BETTER;
		if (hypo->nEnglishFacts() > nEnglishFacts())
			return WORSE;
	}*/

	// We kind of don't want to prefer the most commonly supported descriptions, because
	//   they tend to be very boring. So, we simply prefer those which have an English fact.
	if (nEnglishFacts() > 0 && hypo->nEnglishFacts() == 0)
		return BETTER;
	if (nEnglishFacts() == 0 && hypo->nEnglishFacts() > 0)
		return WORSE;

	DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypo);
	if (descHypoth == DescriptionHypothesis_ptr())
		return false;

	// perhaps we should prefer hypotheses without any coreference resolutions in them?
	// we'd have to look for non-TARGET ENTITY []s

	// Things with more modifiers are more interesting, but things with more than 3 are probably wrong and weird
	if (getNModifiers() < 4) {
		if (getNModifiers() > descHypoth->getNModifiers())
			return BETTER;
		if (getNModifiers() < descHypoth->getNModifiers())
			return WORSE;
	}

	// Prefer the one with the most facts overall, though only consider big differences first
	int supporting_fact_difference = nSupportingFacts() - hypo->nSupportingFacts();
	if (supporting_fact_difference > 4)
		return BETTER;
	if (supporting_fact_difference < -4)
		return WORSE;

	if (isCopula() && !descHypoth->isCopula())
		return BETTER;
	if (descHypoth->isCopula() && !isCopula())
		return WORSE;

	if (isAppositive() && !descHypoth->isAppositive())
		return BETTER;
	if (descHypoth->isAppositive() && !isAppositive())
		return WORSE;

	// Now look at all supporting facts
	if (nSupportingFacts() > hypo->nSupportingFacts())
		return BETTER;
	if (hypo->nSupportingFacts() > nSupportingFacts())
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

bool DescriptionHypothesis::isIllegalHypothesis(ProfileSlot_ptr slot, std::string& rationale) {

	// Basically includes all coreference-derived descriptors
	if (nReliableFacts() == 0) {
		rationale = "unreliable descriptor";
		return true;
	}

	std::wstring display_value = getDisplayValue();

	std::vector<std::wstring> words_with_resolution;
	std::vector<std::wstring> words;
	boost::replace_all(display_value, L"[", L"[ ");
	boost::replace_all(display_value, L"]", L" ]");
	boost::split(words_with_resolution, display_value, boost::is_any_of(L" \t\n"));
	bool in_brackets = false;
	bool all_cap_words = true;
	BOOST_FOREACH(std::wstring word, words_with_resolution) {
		if (word.size() == 0)
			continue;
		else if (word == L"[")
			in_brackets = true;
		else if (word == L"]")
			in_brackets = false;
		else if (!in_brackets) {
			if (iswlower(word.at(0)) && word != L"to" && word != L"of") {
				all_cap_words = false;
			}
			std::transform(word.begin(), word.end(), word.begin(), towlower);	
			words.push_back(word);
		}
	}

	if (words.size() == 0) {
		rationale = "empty";
		return true;
	}

	if (all_cap_words) {
		rationale = "all-caps descriptor";
		return true;
	}

	if (isAppositive()) {
		if (words.at(0) != L"the" && words.at(0) != L"a" && words.at(0) != L"an") {
			rationale = "appositive not starting with a/the";
			return true;
		}
	}

	// Excludes titles from MT. These are pretty risky unless they are conveying 
	//   someone's employment, in which case it belongs in Employment anyway.
	// This also excludes long "titles" as they are almost always bad parses
	if (!isCopula() && !isAppositive()) {
		if (nEnglishFacts() == 0) {
			rationale = "non-English title descriptor";
			return true;
		} 
		if (words.size() > 5) {
			rationale = "too-long title descriptor";
			return true;
		}
	}

	// single words are usually wrong
	if (words.size() == 1) {
		rationale = "single-word descriptor";			
		return true;
	}
	// long descriptions from translation are usually wrong
	if (nEnglishFacts() == 0 && words.size() > 10) {
		rationale = "too-long translated descriptor";			
		return true;
	}

	if (isGenericORGHeadword(_headword) && getMentionArguments().size() == 0 && getNonMentionArguments().size() == 0) {
		rationale = "unmodified generic org";			
		return true;
	}
	
	if (isGenericPERHeadword(_headword) && getMentionArguments().size() == 0 && getNonMentionArguments().size() == 0) {
		rationale = "unmodified generic per";			
		return true;
	}

	// the man, a woman, etc. 
	// this should also include words that are only valuable when modified, e.g. "an advisor"
	if (_headword == L"adult" || 
		_headword == L"advisor" ||
		_headword == L"aide" || 
		_headword == L"ally" || 
		_headword == L"assistant" || 
		_headword == L"author" ||
		_headword == L"beneficiary" || 
		_headword == L"boy" || 
		_headword == L"brother" || 
		_headword == L"capital" || 
		_headword == L"child" || 
		_headword == L"colonel" || 
		_headword == L"company" ||
		_headword == L"cousin" ||
		_headword == L"critic" ||
		_headword == L"daughter" ||
		_headword == L"daughter" || 
		_headword == L"deputy" || 
		_headword == L"elder" ||
		_headword == L"father" || 
		_headword == L"father" || 
		_headword == L"friend" || 
		_headword == L"friend" || 
		_headword == L"general" || 
		_headword == L"girl" || 
		_headword == L"group" || 
		_headword == L"guest" ||
		_headword == L"head" ||
		_headword == L"human" || 
		_headword == L"husband" || 
		_headword == L"leader" || 
		_headword == L"man" || 
		_headword == L"member" || 
		_headword == L"mother" ||
		_headword == L"mother" || 
		_headword == L"one" || 
		_headword == L"opponent" || 
		_headword == L"organization" || 		
		_headword == L"parent" || 
		_headword == L"party" || 
		_headword == L"person" || 
		_headword == L"premier" || 
		_headword == L"president" ||
		_headword == L"role" ||
		_headword == L"sister" || 
		_headword == L"son" || 
		_headword == L"staff" || 
		_headword == L"supporter" ||
		_headword == L"wife" || 
		_headword == L"woman" || 
		_headword == L"younger")
	{
		if (words.size() == 2 || words.size() == 3) {
			std::wstring w1 = words.at(0);
			if (w1 == L"a" || w1 == L"an" || w1 == L"any" || w1 == L"no" || w1 == L"one" ||
				w1 == L"same" || w1 == L"some" || w1 == L"that" || w1 == L"the" ||   w1 == L"these" || 
				w1 == L"this" || w1 == L"those" ||  w1 == L"what" || w1 == L"which" || w1 == L"whose") 
			{
				if (words.size() == 2) {
					rationale = "boring two-word descriptor";			
					return true;
				} else {
					std::wstring w2 = words.at(1);
					if (w2 == L"first" || w2 == L"second" ||  w2 == L"third" || w2 == L"last" || w2 == L"only" ||
						w2 == L"biggest" || w2 == L"smallest" || w2 == L"young") 
					{
						rationale = "boring three-word descriptor";			
						return true;
					}
				}
			}
		}
	}

	if (_headword.compare(L"counterpart") == 0) {
		rationale = "annoying headword";
		return true;
	}

	// MT often translates leadership words weirdly
	// These should really be covered in employment, so we'll disallow them all here
	// We could do this more principled-ly by disallowing Employment-like Descriptions
	// in FactFinder, maybe?
	if (_headword.compare(L"minister") == 0 || _headword.compare(L"minster") == 0 || _headword.compare(L"president") == 0 ||
		_headword.compare(L"executive") == 0 || _headword.compare(L"chairman") == 0 ||
		_headword.compare(L"vice-chairman") == 0 || _headword.compare(L"vice-president") == 0) 
	{
		if (getNModifiers() == 0) {
			rationale = "leader-ly headword";
			return true;
		}
	}
	std::wstring display_value_plus_spaces = L" " + display_value + L" ";

	// "my friend" is not useful, since we can't resolve "my"
	if (display_value_plus_spaces.find(L" my ") != std::wstring::npos || display_value_plus_spaces.find(L" our ") != std::wstring::npos ||
		display_value_plus_spaces.find(L" me ") != std::wstring::npos || display_value_plus_spaces.find(L" we ") != std::wstring::npos ||
		display_value_plus_spaces.find(L" its ") != std::wstring::npos || display_value_plus_spaces.find(L" it ") != std::wstring::npos) 
	{
		rationale = "un-resolved (or likely poorly resolved) pronouns";
		return true;
	}
	
	// description  that start or end in ellipsis most likely came from a truncated sentence.
	if (display_value.rfind(L"...") == (display_value.length()-3) ||
		display_value.find(L"...") == 0){
		rationale = "truncated description";
		return true;
	}

	if (words.size() > 0) {
		std::wstring last_word = words.at(words.size() - 1);
		if (last_word == L"and" || last_word == L"or" || last_word == L"of") {
			rationale = "truncated description";
			return true;
		}
	}

	if (display_value == L"no one" || display_value == L"no-one") {
		rationale = "boring";
		return true;
	}

	// Consider changing FF to output [NO_RESOLUTION] for unresolved pronouns, including
	// third-person pronouns, so we can know not to output them. We can't just check to 
	// see if the pronoun is unresolved, since it would cancel things like "a man running
	// in his first election"

	return false;

}

bool DescriptionHypothesis::isSpecialIllegalHypothesis(std::string& rationale,
	std::vector<NameHypothesis_ptr>& namesAlreadyDisplayed,
	std::vector<std::wstring>& titlesAlreadyDisplayed,
	std::vector<NameHypothesis_ptr>& nationalitiesAlreadyDisplayed,
	NameHypothesis::GenderType gender)
{
	BOOST_FOREACH(NameHypothesis_ptr nameHypo, getMentionArguments()) {
		BOOST_FOREACH(NameHypothesis_ptr nameHypoAlreadySeen, namesAlreadyDisplayed) {
			// ??? should this be isSimilar test for name here?
			if (nameHypo->isEquiv(nameHypoAlreadySeen) &&
				!nameHypo->isPersonSlot())
			{
				rationale = "intra-profile repeated non-person entity";		
				return true;
			}
		}
		/* When used to restrict PER descriptions, this was too restrictive,
		 *  tossing many first-time references to places that were indeed good.
		if (nameHypo->getSupportingFacts().at(0)->answer_entity_type.compare("GPE") == 0) {
			bool has_match = false;
			BOOST_FOREACH(NameHypothesis_ptr natHypo, nationalitiesAlreadyDisplayed) {
				if (natHypo->isEquiv(nameHypo)) {
					has_match = true;
				}
			}
			if (!has_match) {
				rationale = "conflicting GPE modifier(too dangerous)";	
				return true;
			}
		}
		*/
	}	

	BOOST_FOREACH(std::wstring title, titlesAlreadyDisplayed) {
		if (title == _headword) {
			rationale = "intra-profile repeated headword";			
			return true;
		}
	}

	if (gender == NameHypothesis::MALE) {
		loadGenderDescLists();
		if (_femaleDescriptions.find(_headword) != _femaleDescriptions.end()) {
			rationale = "mismatched gender description";
			return true;
		}
	} else if (gender == NameHypothesis::FEMALE) {
		loadGenderDescLists();
		if (_maleDescriptions.find(_headword) != _maleDescriptions.end()) {
			rationale = "mismatched gender description";
			return true;
		}
	}
	return false;
}



bool DescriptionHypothesis::isRiskyHypothesis(ProfileSlot_ptr slot, std::string& rationale) {
	if (!isCopula() && nEnglishFacts() == 0) {
		rationale = "MT non-copula (risky)";
		return true;
	}
	return false;
}
	

void DescriptionHypothesis::gatherExistingProfileInformation(Profile_ptr existing_profile,
																	std::vector<NameHypothesis_ptr>& namesAlreadyDisplayed,
																	std::vector<std::wstring>& titlesAlreadyDisplayed,
																	std::vector<NameHypothesis_ptr>& nationalitiesAlreadyDisplayed) 
{
	if (existing_profile == Profile_ptr())
		return;
	std::pair<std::string, ProfileSlot_ptr> typeSlotPair;
	BOOST_FOREACH(typeSlotPair, existing_profile->getSlots()) {
		std::list<GenericHypothesis_ptr> outputHypotheses = typeSlotPair.second->getOutputHypotheses();
		BOOST_FOREACH(GenericHypothesis_ptr hypo, outputHypotheses) {
			NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(hypo);
			if (nameHypoth != NameHypothesis_ptr()) {				
				if (typeSlotPair.first == "nationality")
					nationalitiesAlreadyDisplayed.push_back(nameHypoth);
				else namesAlreadyDisplayed.push_back(nameHypoth);
			}
			EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypo);
			if (empHypoth != EmploymentHypothesis_ptr() && empHypoth->getNamedArgument() != NameHypothesis_ptr() && 
				empHypoth->isEmployerHypothesis()) 
			{
				namesAlreadyDisplayed.push_back(empHypoth->getNamedArgument());
				std::wstring job_title = empHypoth->getJobTitle();
				std::transform(job_title.begin(), job_title.end(), job_title.begin(), towlower);
				titlesAlreadyDisplayed.push_back(job_title);

				// also take the last word in the job title
				size_t last_space_index = job_title.rfind(L" ");
				if (last_space_index != std::wstring::npos && last_space_index + 1 < job_title.size()) {
					job_title = job_title.substr(last_space_index + 1);
					titlesAlreadyDisplayed.push_back(job_title);
				}				
			}
		}
	}
}
