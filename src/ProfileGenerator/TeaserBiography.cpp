// Copyright (c) 2011 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"

#include "ProfileGenerator/TeaserBiography.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"

#include "boost/foreach.hpp"
#include <boost/algorithm/string.hpp> 
#include <list>
#include <iostream>

TeaserBiography::TeaserBiography(Profile_ptr profile) {

	_biography = L"";
	_profile = profile;
	_name_hypothesis = profile->getProfileNameHypothesis();
	_bio_mentions = 0;
	if (profile->getProfileType() == Profile::PER) {
		_biography += getLeadPersonSentence(profile);
		_biography += getFamilySentence(profile);
		_biography += getEducationSentence(profile);
		_biography += getDescriptionSentence(profile);
	} else if (profile->getProfileType() == Profile::ORG) {
		_biography += getLeadOrganizationSentence(profile);
		_biography += getDescriptionSentence(profile);
	} else if (profile->getProfileType() == Profile::GPE) {
		// TODO
	}

}
std::wstring TeaserBiography::repeatReference(NameHypothesis::PronounRole role, bool capitalize){
	std::wstring ref = _name_hypothesis->getRepeatReference(_bio_mentions, role);
	if (ref != L"" && capitalize)
		ref.at(0) = towupper(ref.at(0));
	return ref;
}

std::wstring TeaserBiography::getLeadPersonSentence(Profile_ptr profile) {	

	std::wstring birth_clause = getDateDisplayValue(profile->getExistingSlot(ProfileSlot::BIRTHDATE_UNIQUE_ID));
	std::wstring death_clause = getDateDisplayValue(profile->getExistingSlot(ProfileSlot::DEATHDATE_UNIQUE_ID));
	std::wstring nationality = L"";
	std::wstring job_title = L"";
	std::wstring employer = L"";
	ProfileSlot_ptr nat_slot = profile->getExistingSlot(ProfileSlot::NATIONALITY_UNIQUE_ID);
	if (nat_slot != ProfileSlot_ptr() && nat_slot->getOutputHypotheses().size() != 0)
		nationality = nat_slot->getOutputHypotheses().front()->getDisplayValue();
	ProfileSlot_ptr employer_slot = profile->getExistingSlot(ProfileSlot::EMPLOYER_UNIQUE_ID);
	EmploymentHypothesis::status_tense_t employment_tense;
	if (employer_slot != ProfileSlot_ptr() && employer_slot->getOutputHypotheses().size() != 0) {
		EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(employer_slot->getOutputHypotheses().front());
		if (empHypoth->getNamedArgument() != NameHypothesis_ptr()) {
			employer = empHypoth->getNamedArgument()->getDisplayValue();
			job_title = empHypoth->getJobTitle();
			if (job_title == L"ceo" || job_title == L"cio" || job_title == L"coo" || job_title == L"cto" || job_title == L"mp")
				std::transform(job_title.begin(), job_title.end(), job_title.begin(), towupper);				
			employment_tense = empHypoth->getTense(true); // temporarily print debug info
		}
	}

	if (nationality == L"" && employer == L"") {
		std::wstring ref = repeatReference(NameHypothesis::SUBJECT, true);
		if (birth_clause != L"" && death_clause != L"") {
			return ref + L" was born " + birth_clause + L" and died " + death_clause + L". ";
		} else if (birth_clause != L"") {
			return ref + L" was born " + birth_clause + L". ";
		} else if (death_clause != L"") {
			return ref + L" died " + death_clause + L". ";
		} 
		_bio_mentions--; // we didn't use this references, so decrement the counter
		return L"";
	} 

	bool is_alive = (death_clause == L"");
	bool dangling = false;

	std::wstring ret_value = L"";
	if (birth_clause != L"") {
		// We know there is either a nationality or employer coming, since they aren't both null
		ret_value += L"Born " + birth_clause + L", " + repeatReference(NameHypothesis::SUBJECT, false);
		dangling = true;
	} 

	if (nationality != L"") {
		if (employer == L"") {
			// All we have is nationality and possibly death, so fix up the sentence and return.
			if (!dangling) {
				ret_value = repeatReference(NameHypothesis::SUBJECT, true);
				dangling = true;
			}
			if (death_clause != L"")
				ret_value += L" was a citizen of " + nationality + L" who died " + death_clause + L". ";
			else ret_value += L" is a citizen of " + nationality + L". ";
			dangling = false; // not that it matters
			return ret_value;
		} else if (nationality != employer) {
			// We know we have employment coming, so mention nationality and continue
			if (!dangling) {
				ret_value += L"A citizen of " + nationality + L", " + repeatReference(NameHypothesis::SUBJECT, false);			
				dangling = true;
			} else {
				if (is_alive)
					ret_value += L" is";
				else ret_value += L" was";
				ret_value += L" a citizen of " + nationality + L" and ";
				dangling = true;
			}
		} 
	}

	// We know we have an employer, otherwise we would have returned above		
	if (!dangling){
		ret_value += repeatReference(NameHypothesis::SUBJECT, true);
		dangling = true;
	}

	if (job_title != L"") {
		if (!is_alive || employment_tense == EmploymentHypothesis::PAST)
			ret_value += L" was ";
		else if (employment_tense == EmploymentHypothesis::FUTURE)
			ret_value += L" is reportedly the future ";
		else 
			ret_value += L" is ";
		ret_value += getJobTitleArticle(job_title) + L" " + job_title + L" of " + modifyEmployerName(employer);
	} else {
		if (!is_alive || employment_tense == EmploymentHypothesis::PAST)
			ret_value += L" used to work for ";
		else if (employment_tense == EmploymentHypothesis::FUTURE)
			ret_value += L" reportedly will work for ";
		else 
			ret_value += L" works for ";
		ret_value += modifyEmployerName(employer);
	}
	ret_value += L". ";

	if (death_clause != L"") {
		ret_value += repeatReference(NameHypothesis::SUBJECT, true) + L" died " + death_clause + L". ";
	}

	return ret_value;
}

std::wstring TeaserBiography::modifyEmployerName(std::wstring employer) {
	// TODO_IDEA: case?
	if (employer == L"United States" || employer == L"Senate" || employer == L"House of Representatives" ||
		employer == L"House" ||
		employer == L"United Kingdom" || employer == L"United Nations")
		return L"the " + employer;
	if (employer == L"UN")
		return L"the United Nations";
	if (employer == L"State")
		return L"the State Department";
	if (employer == L"Defense")
		return L"the Defense Department";
	return employer;
}

bool TeaserBiography::isSingularJobTitle(std::wstring title) {
	if (title == L"president" || title == L"vice president" || title == L"chancellor" || title == L"leader" || title == L"ceo" ||
		title == L"chairman" || title == L"head" || title == L"premier" || title == L"dictator" || title == L"treasurer" ||
		title == L"governor" || title == L"mayor" || title == L"coo" || title == L"chairwoman" || title == L"chairperson" ||
		title == L"chair" || title == L"co-chair" || title == L"founder" || title == L"provost" || title == L"chancellor" ||
		title == L"dean" || title == L"speaker" || title == L"commissioner" || title == L"commissar" || title == L"commander in chief" ||
		title == L"cio" || title == L"cto" ||
		title == L"CEO" || title == L"CIO" || title == L"COO" || title == L"CTO" || title == L"MP")				
		return true;

	if (title.find(L"minister") != std::wstring::npos ||
		title.find(L"president") != std::wstring::npos ||
		title.find(L"executive") != std::wstring::npos ||
		title.find(L"director") != std::wstring::npos ||
		title.find(L"head") != std::wstring::npos ||
		title.find(L"chief") != std::wstring::npos ||
		title.find(L"general") != std::wstring::npos ||
		title.find(L"co-") != std::wstring::npos ||
		title.find(L"vice") != std::wstring::npos ||
		title.find(L"deputy") != std::wstring::npos)
		return true;

	return false;
}

std::wstring TeaserBiography::getJobTitleArticle(std::wstring title) {
	if (isSingularJobTitle(title))
		return L"the";

	if (title.at(0) == L'a' || title.at(0) == L'e' || title.at(0) == L'i' || title.at(0) == L'o' || title.at(0) == L'u')
		return L"an";

	if (title == L"mp")
		return L"an";

	return L"a";
}

std::wstring TeaserBiography::getFamilySentence(Profile_ptr profile) {
	std::wstring spouse_list = getPrintableList(profile->getExistingSlot(ProfileSlot::SPOUSE_UNIQUE_ID), 1, false);
	std::wstring family_list = getPrintableList(profile->getExistingSlot(ProfileSlot::FAMILY_UNIQUE_ID), 3, false);
	std::wstring death_date = getDateDisplayValue(profile->getExistingSlot(ProfileSlot::DEATHDATE_UNIQUE_ID));
	bool is_alive = death_date == L"";
	std::wstring to_be_tensed = is_alive ? L" is " : L" was ";
	if (spouse_list == L"" && family_list == L"") {
		return L"";
	} else if (spouse_list == L"") {
		return L"Family members include " + family_list + L". "; 
	} else if (family_list == L"") {
		return repeatReference(NameHypothesis::SUBJECT, true) + to_be_tensed + L"married to " + spouse_list + L". "; 
	} else {
		return repeatReference(NameHypothesis::SUBJECT, true) + to_be_tensed + L"married to " + spouse_list + L"; other family members include " + family_list + L". "; 
	}
	return L"";
}

std::wstring TeaserBiography::getEducationSentence(Profile_ptr profile) {
	std::wstring schools = getPrintableList(profile->getExistingSlot(ProfileSlot::EDUCATION_UNIQUE_ID), 3, false);
	if (schools != L"")
		return repeatReference(NameHypothesis::SUBJECT, true) + L" was educated at " + schools + L". ";
	else return L"";
}

std::wstring TeaserBiography::getLeadOrganizationSentence(Profile_ptr profile) {
	std::wstring hq = getPrintableList(profile->getExistingSlot(ProfileSlot::HEADQUARTERS_UNIQUE_ID), 3, false);
	std::wstring founder = getPrintableList(profile->getExistingSlot(ProfileSlot::FOUNDER_UNIQUE_ID), 1, false);
	std::wstring founding_date = getDateDisplayValue(profile->getExistingSlot(ProfileSlot::FOUNDINGDATE_UNIQUE_ID));

	// Separate out former and current leaders
	std::list<GenericHypothesis_ptr> leader_hypotheses;
	std::list<GenericHypothesis_ptr> former_leader_hypotheses;
	ProfileSlot_ptr leaders_slot = profile->getExistingSlot(ProfileSlot::LEADER_UNIQUE_ID);
	if (leaders_slot != ProfileSlot_ptr()) {
		BOOST_FOREACH(GenericHypothesis_ptr hypoth, leaders_slot->getOutputHypotheses()) {
			EmploymentHypothesis::status_tense_t tense = getEmploymentTense(hypoth);
			if (tense == EmploymentHypothesis::CURRENT)
				leader_hypotheses.push_back(hypoth);
			else if (tense == EmploymentHypothesis::PAST)
				former_leader_hypotheses.push_back(hypoth);
		}
	}
	std::wstring leader_list = getPrintableList(leader_hypotheses, 2, true, false);
	std::wstring former_leader_list = getPrintableList(former_leader_hypotheses, 2, true, false);
	boost::replace_all(former_leader_list, L")", L"");
	boost::replace_all(former_leader_list, L"(", L"");

	// Separate out former and current employees
	std::list<GenericHypothesis_ptr> employee_hypotheses;
	std::list<GenericHypothesis_ptr> former_employee_hypotheses;
	ProfileSlot_ptr employees_slot = profile->getExistingSlot(ProfileSlot::EMPLOYEE_UNIQUE_ID);
	if (employees_slot != ProfileSlot_ptr()) {
		BOOST_FOREACH(GenericHypothesis_ptr hypoth, employees_slot->getOutputHypotheses()) {
			EmploymentHypothesis::status_tense_t tense = getEmploymentTense(hypoth);
			if (tense == EmploymentHypothesis::CURRENT)
				employee_hypotheses.push_back(hypoth);
			else if (tense == EmploymentHypothesis::PAST)
				former_employee_hypotheses.push_back(hypoth);
		}
	}
	std::wstring employee_list = getPrintableList(employee_hypotheses, 5, false, false);
	std::wstring former_employee_list = getPrintableList(former_employee_hypotheses, 3, false, false);

	std::wstring ret_value = L"";

	ProfileSlot_ptr employeeSlot = profile->getExistingSlot(ProfileSlot::EMPLOYEE_UNIQUE_ID);
	int n_employees = 0;
	if (employeeSlot != ProfileSlot_ptr())
		n_employees = static_cast<int>(employeeSlot->getOutputHypotheses().size());

	if (leader_list == L"" && former_leader_list == L"" && employee_list == L"")
		return getSimpleFoundingHQSentence(founder, founding_date, hq);

	bool has_preceding_clause = true;
	if (founder != L"" && founding_date != L"")
		ret_value += L"Founded by " + founder + L" " + founding_date + L", ";
	else if (founder != L"") 
		ret_value += L"Founded by " + founder + L", ";
	else if (founding_date != L"")
		ret_value += L"Founded " + founding_date + L", ";
	else has_preceding_clause = false;

	// Just do leaders if we have some
	if (leader_list != L"") {		
		ret_value += repeatReference(NameHypothesis::SUBJECT, !has_preceding_clause) + L" is led by " + leader_list + L". ";
		if (former_leader_list != L"") 			
			ret_value += L"(Former leaders include " + former_leader_list + L".) ";
	} else if (former_leader_list != L"") {		
		ret_value += repeatReference(NameHypothesis::SUBJECT, !has_preceding_clause) + L" has been led by " + former_leader_list + L". ";
	} else if (employee_list != L"") {
		ret_value += repeatReference(NameHypothesis::POSSESSIVE_MOD, !has_preceding_clause) + L" employees/members include " + employee_list + L". ";
		if (former_employee_list != L"") 
			ret_value += L"(Additional former employees/members: " + former_employee_list + L".) ";
	}

	if (hq != L"")
		ret_value += repeatReference(NameHypothesis::SUBJECT, true) + L" has headquarters in " + hq + L". ";

	return ret_value;	
}


std::wstring TeaserBiography::getSimpleFoundingHQSentence(std::wstring founder, std::wstring founding_date, std::wstring hq) {
	std::wstring founding_sentence = L"";
	if (founder != L"" && founding_date != L"")
		founding_sentence = repeatReference(NameHypothesis::SUBJECT, true) + L" was founded by " + founder + L" " + founding_date ;
	else if (founder != L"") 
		founding_sentence = repeatReference(NameHypothesis::SUBJECT, true) + L" was founded by " + founder;
	else if (founding_date != L"")
		founding_sentence = repeatReference(NameHypothesis::SUBJECT, true) + L" was founded " + founding_date;
	
	if (hq != L"") {
		if (founding_sentence != L"")
			return founding_sentence + L" and has headquarters in " + hq + L". ";
		else return repeatReference(NameHypothesis::SUBJECT, true) + L" has headquarters in " + hq + L". ";
	} else if (founding_sentence != L"") {
		return founding_sentence + L". ";
	}

	return L"";
}

std::wstring TeaserBiography::getDescriptionSentence(Profile_ptr profile) {
	ProfileSlot_ptr slot = profile->getExistingSlot(ProfileSlot::DESCRIPTION_UNIQUE_ID);
	if (slot == ProfileSlot_ptr())
		return L"";
	std::list<GenericHypothesis_ptr> hypotheses;
	BOOST_FOREACH(GenericHypothesis_ptr hypoth, slot->getOutputHypotheses()) {
		if (hypoth->getDisplayValue().size() <= 100)
			hypotheses.push_back(hypoth);
	}
	std::wstring descriptions = getPrintableList(hypotheses, 4, false, true);
	if (descriptions != L"") {
		descriptions = profile->stripOfResolutionsToThisActor(descriptions);
		return repeatReference(NameHypothesis::SUBJECT, true) + L" has been described as " + descriptions + L". ";
	}
	else return L"";
}

std::wstring TeaserBiography::getPrintableList(ProfileSlot_ptr slot, int max_to_print, bool include_titles) {
	if (slot == ProfileSlot_ptr())
		return L"";
	return getPrintableList(slot->getOutputHypotheses(), max_to_print, include_titles, false);
}

std::wstring TeaserBiography::getPrintableList(std::list<GenericHypothesis_ptr>& hypotheses, int max_to_print, bool include_titles, bool is_description) {

	if (hypotheses.size() == 0 || max_to_print == 0) {
		return L"";
	} else if (hypotheses.size() == 1 || max_to_print == 1) {		
		return getDisplayValue(hypotheses.front(), include_titles, is_description);
	} else if (hypotheses.size() == 2  || max_to_print == 2) {
		std::wstring ret_value = L"";
		int counter = 0;
		for (std::list<GenericHypothesis_ptr>::iterator iter = hypotheses.begin(); iter != hypotheses.end(); iter++) {
			GenericHypothesis_ptr hypoth = *iter;
			ret_value += getDisplayValue(hypoth, include_titles, is_description);	
			if (counter == 0)
				ret_value += L" and ";
			else if (counter == 1)
				break;		
			counter++;
		}		
		return ret_value;
	} else {
		std::wstring ret_value = L"";
		size_t max = max_to_print;
		if (hypotheses.size() < max)
			max = hypotheses.size();
		size_t counter = 0;
		for (std::list<GenericHypothesis_ptr>::iterator iter = hypotheses.begin(); iter != hypotheses.end(); iter++) {
			GenericHypothesis_ptr hypoth = *iter;
			ret_value += getDisplayValue(hypoth, include_titles, is_description);
			if (counter < max - 2)
				ret_value += L", ";
			else if (counter < max - 1)
				ret_value += L", and ";			
			counter++;
			if (counter == max)
				break;
		}
		return ret_value;
	}
}

std::wstring TeaserBiography::getDisplayValue(GenericHypothesis_ptr hypo, bool include_titles, bool use_quotes) {

	std::wstring ret_value = hypo->getDisplayValue();

	// Special handling for employees
	EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypo);
	if (empHypoth != EmploymentHypothesis_ptr()) {
		if (empHypoth->getNamedArgument() == NameHypothesis_ptr())
			return L""; // this should never happen
		ret_value = empHypoth->getNamedArgument()->getDisplayValue();
		if (include_titles) {
			std::wstring title = empHypoth->getJobTitle();
			if (title.size() != 0) {
				if (title == L"ceo" || title == L"cio" || title == L"coo" || title == L"cto" || title == L"mp")
					std::transform(title.begin(), title.end(), title.begin(), towupper);
				if (title != L"leader" && title != L"head" && title != L"leaders" && title != L"heads") {
					if (isSingularJobTitle(title))
						ret_value += L" (as " + title + L")";
					else ret_value += L" (as " + getJobTitleArticle(title) + L" " + title + L")";
				}
			}
		}
	}
	
	if (use_quotes)
		return L"\"" + ret_value + L"\"";
	else return ret_value;
}

EmploymentHypothesis::status_tense_t TeaserBiography::getEmploymentTense(GenericHypothesis_ptr hypo) {

	EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypo);
	if (empHypoth != EmploymentHypothesis_ptr()) {
		return empHypoth->getTense(true); // temporarily print debug info
	}
	return EmploymentHypothesis::CURRENT;	
}


std::wstring TeaserBiography::getDateDisplayValue(ProfileSlot_ptr slot) {
	if (slot == ProfileSlot_ptr() || slot->getOutputHypotheses().size() == 0)
		return L"";
	std::wstring value = slot->getOutputHypotheses().front()->getDisplayValue();
	if (value.length() == 4)
		return L"in " + value;
	else if (value.length() == 7)
		return L"in " + monthFromIntWString(value.substr(5,2)) + L" of " + value.substr(0,4);
	else if (value.length() == 8 && value.at(5) == L'W')
		return L"in " + value.substr(0,4);
	else if (value.length() == 10){
		std::wstring dayStr = value.substr(8,2);
		if (dayStr.substr(0,1) == L"0")
			dayStr = dayStr.substr(1,1);
		return L"on " + monthFromIntWString(value.substr(5,2)) + L" " + dayStr + L", " + value.substr(0,4);
	}
	//this one is not supposed  to happen, but ....
	SessionLogger::warn("unrecog_date_0") << L"unrecognized datestring " << value;
	return value;  
}
std::wstring TeaserBiography::monthFromIntWString(std::wstring monthInt) {
	if (monthInt == L"01")
		return L"January";
	else if (monthInt == L"02")
		return L"February";
	else if (monthInt == L"03")
		return L"March";
	else if (monthInt == L"04")
		return L"April";
	else if (monthInt == L"05")
		return L"May";
	else if (monthInt == L"06")
		return L"June";
	else if (monthInt == L"07")
		return L"July";
	else if (monthInt == L"08")
		return L"August";
	else if (monthInt == L"09")
		return L"September";
	else if (monthInt == L"10")
		return L"October";
	else if (monthInt == L"11")
		return L"November";
	else if (monthInt == L"12")
		return L"December";

	// might as well log the bug
	SessionLogger::warn("unrecog_date_0") << L"unrecognized datestring for month integer" << monthInt;
	return monthInt;



}
