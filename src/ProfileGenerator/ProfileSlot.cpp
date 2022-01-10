// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/ParamReader.h"
#include "Generic/common/Attribute.h"
#include "Generic/common/UnicodeUtil.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/DateHypothesis.h"
#include "ProfileGenerator/DescriptionHypothesis.h"
#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/PhraseHypothesis.h"
#include "ProfileGenerator/SimpleStringHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"
#include "ProfileGenerator/ConditionHypothesis.h"
#include "ProfileGenerator/PGDatabaseManager.h"
#include "ProfileGenerator/PGFactDate.h"
#include "ProfileGenerator/DateDescription.h"

#include "boost/foreach.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/assign/std/vector.hpp"
using namespace boost::assign;
using namespace std;

#include <iostream>
#include <sstream>
#include <string>
#include <list>

#include "Generic/state/XMLStrings.h"
using namespace SerifXML;

const int ProfileSlot::SMALL_PROFILE_FACTS_COUNT = 10;

const std::string ProfileSlot::ACTIONS_UNIQUE_ID =  "actions"; 
const std::string ProfileSlot::ASSOCIATE_UNIQUE_ID =  "associate"; 
const std::string ProfileSlot::BIRTHDATE_UNIQUE_ID =  "birthdate"; 
const std::string ProfileSlot::DEATHDATE_UNIQUE_ID =  "deathdate"; 
const std::string ProfileSlot::DESCRIPTION_UNIQUE_ID =  "description"; 
const std::string ProfileSlot::EDUCATION_UNIQUE_ID =  "education"; 
const std::string ProfileSlot::EMPLOYEE_UNIQUE_ID =  "employee"; 
const std::string ProfileSlot::EMPLOYER_UNIQUE_ID =  "employer"; 
const std::string ProfileSlot::FAMILY_UNIQUE_ID =  "family"; 
const std::string ProfileSlot::FOUNDER_UNIQUE_ID =  "founder"; 
const std::string ProfileSlot::FOUNDINGDATE_UNIQUE_ID =  "founding_date"; 
const std::string ProfileSlot::HEADQUARTERS_UNIQUE_ID =  "headquarters"; 
const std::string ProfileSlot::LEADER_UNIQUE_ID =  "leader"; 
const std::string ProfileSlot::NATIONALITY_UNIQUE_ID =  "nationality"; 
const std::string ProfileSlot::QUOTES_ABOUT_UNIQUE_ID =  "quotes_about"; 
const std::string ProfileSlot::QUOTES_BY_UNIQUE_ID =  "quotes_by"; 
const std::string ProfileSlot::SPOUSE_UNIQUE_ID =  "spouse"; 
const std::string ProfileSlot::TEASER_BIO_UNIQUE_ID =  "teaser_bio"; 	
const std::string ProfileSlot::VISIT_UNIQUE_ID =  "visit"; 

const std::string ProfileSlot::KB_EMPLOYMENT_FACT_TYPE = "Employment";
const std::string ProfileSlot::KB_EMPLOYEE_ROLE = "Employee";
const std::string ProfileSlot::KB_EMPLOYER_ROLE = "Employer";

ProfileSlot::ProfileSlot(xercesc::DOMElement* slot, PGDatabaseManager_ptr pgdm)
{

	// This can be changed when debugging if desired, but I think when we print HTML, we generally want this true
	_print_all_hypotheses_to_html = true;
	_output_up_to_date = false;
	
	std::stringstream errstr;	
	
	if (slot->hasAttribute(X_unique_id)) {
		_unique_id = SerifXML::transcodeToStdString(slot->getAttribute(X_unique_id));
	} else {
		errstr << "Profile slot missing required attribute 'unique_id'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	if (slot->hasAttribute(X_display_name))
		_displayName = SerifXML::transcodeToStdString(slot->getAttribute(X_display_name));
	else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'display_name'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	if (slot->hasAttribute(X_display_type)) {
		_displayType = SerifXML::transcodeToStdString(slot->getAttribute(X_display_type));
		if (_displayType != "named_entity" && _displayType != "text") {
			errstr << "Profile slot '" << _unique_id << "' has invalid attribute 'display_type'; must be 'text' or 'named_entity'";
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
		}
	} else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'display_type'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}
	
	if (slot->hasAttribute(X_hypothesis_type)) {
		_hypothesisType = SerifXML::transcodeToStdString(slot->getAttribute(X_hypothesis_type));
		if (_hypothesisType != "name" && _hypothesisType != "phrase" &&	_hypothesisType != "date" && 
			_hypothesisType != "description" && _hypothesisType != "employment" && _hypothesisType != "condition"  && _hypothesisType != "simple_string") 
		{
			errstr << "Profile slot '" << _unique_id << "' has invalid attribute 'hypothesis_type'; must be 'phrase', 'name', 'date', 'description', 'simple_string', 'condition', or 'employment'";
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
		}
	} else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'hypothesis_type'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	if (slot->hasAttribute(X_kb_fact_type)) {
		std::string kb_fact_type = SerifXML::transcodeToStdString(slot->getAttribute(X_kb_fact_type));
		_kb_fact_type_id = pgdm->convertStringToId("FactType", kb_fact_type, true);
	} else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'X_kb_fact_type'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}
	
	if (slot->hasAttribute(X_focus_role))
		_focus_role = SerifXML::transcodeToStdString(slot->getAttribute(X_focus_role));
	else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'focus_role'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	if (slot->hasAttribute(X_answer_role))
		_answer_role = SerifXML::transcodeToStdString(slot->getAttribute(X_answer_role));
	else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'answer_role'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	if (slot->hasAttribute(X_sort_order)) {
		_sortOrder = SerifXML::transcodeToStdString(slot->getAttribute(X_sort_order));
		if (_sortOrder != "count" && _sortOrder != "date") {
			errstr << "Profile slot '" << _unique_id << "' sort_order must be 'date' or 'count'";
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
		}
	} else {
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'sort_order'";
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", errstr.str().c_str());
	}

	xercesc::DOMNodeList* factSpecs = slot->getElementsByTagName(X_fact);
	for (size_t i = 0; i < factSpecs->getLength(); i++) {
		xercesc::DOMElement* fact_spec = dynamic_cast<xercesc::DOMElement*>(factSpecs->item(i));
		addDatabaseFactInfo(fact_spec, pgdm);
	}
	
	if (slot->hasAttribute(X_is_infobox) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_is_infobox)))
		_is_infobox = true;
	else 
		_is_infobox = false;	
	
	if (slot->hasAttribute(X_is_binary_relation) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_is_binary_relation)))
		_is_binary_relation = true;
	else 
		_is_binary_relation = false;		
	
	if (slot->hasAttribute(X_is_person) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_is_person)))
		_is_person = true;
	else 
		_is_person = false;
	
	if (slot->hasAttribute(X_is_family) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_is_family)))
		_is_family = true;
	else 
		_is_family = false;
	
	if (slot->hasAttribute(X_is_location) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_is_location)))
		_is_location = true;
	else 
		_is_location = false;
	
	if (slot->hasAttribute(X_use_spokespeople) && "true" == SerifXML::transcodeToStdString(slot->getAttribute(X_use_spokespeople)))
		_use_spokespeople = true;
	else 
		_use_spokespeople = false;
	
	if (slot->hasAttribute(X_rank)) {
		std::string temp = SerifXML::transcodeToStdString(slot->getAttribute(X_rank));
		try {
			_rank = boost::lexical_cast<int>(temp);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", "Bad rank attribute", temp.c_str());
		}
	} else {
		throw UnexpectedInputException("ProfileSlot::ProfileSlot", "Missing rank attribute; this should be added by the C++ code??");
	}

	if (slot->hasAttribute(X_min_facts_ratio)) {
		std::string temp = SerifXML::transcodeToStdString(slot->getAttribute(X_min_facts_ratio));
		try {
			_min_facts_ratio = boost::lexical_cast<float>(temp);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", "Bad min_facts_ratio attribute", temp.c_str());
		}
	} else _min_facts_ratio = 0;

	if (slot->hasAttribute(X_max_hypotheses)) {
		std::string temp = SerifXML::transcodeToStdString(slot->getAttribute(X_max_hypotheses));
		try {
			_max_uploaded_hypoth = boost::lexical_cast<int>(temp);
		} catch (boost::bad_lexical_cast &) {
			throw UnexpectedInputException("ProfileSlot::ProfileSlot", "Bad max_hypotheses attribute", temp.c_str());
		}
	} else _max_uploaded_hypoth = -1;

	if (ParamReader::isParamTrue("cutoff_responses")) {
		if (slot->hasAttribute(X_max_hypotheses_if_cutoff)) {
			std::string temp = SerifXML::transcodeToStdString(slot->getAttribute(X_max_hypotheses_if_cutoff));
			try {
				_max_uploaded_hypoth = boost::lexical_cast<int>(temp);
			} catch (boost::bad_lexical_cast &) {
				throw UnexpectedInputException("ProfileSlot::ProfileSlot", "Bad max_hypotheses_if_cutoff attribute", temp.c_str());
			}
		} // else use what it was before
	}

}

void ProfileSlot::addDatabaseFactInfo(xercesc::DOMElement* elt, PGDatabaseManager_ptr pgdm) {
	
	DatabaseFactInfo dfi;
	std::stringstream errstr;

	if (elt->hasAttribute(X_fact_type)) {
		std::string fact_type_str = SerifXML::transcodeToStdString(elt->getAttribute(X_fact_type));
		dfi.fact_type = pgdm->convertStringToId("FactType", fact_type_str, false);
	} else {
		errstr << "Profile slot '" << _unique_id << "' 'fact' element missing required attribute 'fact_type'";
		throw UnexpectedInputException("ProfileSlot::addDatabaseFactInfo", errstr.str().c_str());
	}

	if (elt->hasAttribute(X_extractor_name) && elt->hasAttribute(X_kb_source)) {
		errstr << "Profile slot '" << _unique_id << "' 'fact' element must specify exactly one (not both) of 'extractor_name' or 'kb_source'";
		throw UnexpectedInputException("ProfileSlot::addDatabaseFactInfo", errstr.str().c_str());
	}

	if (elt->hasAttribute(X_extractor_name)) {
		std::string source_str = SerifXML::transcodeToStdString(elt->getAttribute(X_extractor_name));
		dfi.source_info = std::make_pair<db_fact_type_t, int>(DOCUMENT, pgdm->convertStringToId("Extractor", source_str, false));
	} else if (elt->hasAttribute(X_kb_source)) {
		std::string source_str = SerifXML::transcodeToStdString(elt->getAttribute(X_kb_source));
		dfi.source_info = std::make_pair<db_fact_type_t, int>(EXTERNAL, pgdm->convertStringToId("Source", source_str, false));
	} else {
		errstr << "Profile slot '" << _unique_id << "' 'fact' element missing required attribute 'extractor_name' or 'kb_source' (exactly one should be specified)";
		throw UnexpectedInputException("ProfileSlot::addDatabaseFactInfo", errstr.str().c_str());
	}

	if (elt->hasAttribute(X_focus_role)) {
		dfi.focus_role = SerifXML::transcodeToStdString(elt->getAttribute(X_focus_role));
		dfi.kb_role_map[dfi.focus_role] = _focus_role;
	}
	else {		
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'focus_role'";
		throw UnexpectedInputException("ProfileSlot::addDatabaseFactInfo", errstr.str().c_str());
	}

	if (elt->hasAttribute(X_answer_role)) {
		dfi.answer_role = SerifXML::transcodeToStdString(elt->getAttribute(X_answer_role));
		dfi.kb_role_map[dfi.answer_role] = _answer_role;
	}
	else {		
		errstr << "Profile slot '" << _unique_id << "' missing required attribute 'answer_role'";
		throw UnexpectedInputException("ProfileSlot::addDatabaseFactInfo", errstr.str().c_str());
	}

	if (elt->hasAttribute(X_other_roles)) {
		std::string roles_raw = SerifXML::transcodeToStdString(elt->getAttribute(X_other_roles));
		std::vector<std::string> roles;
		boost::split(roles, roles_raw, boost::is_any_of(";"));
		BOOST_FOREACH(std::string role, roles) {
			addRoleMapping(dfi, role);
		}
	}

	_databaseFactInfoList.push_back(dfi);
}

void ProfileSlot::addRoleMapping(ProfileSlot::DatabaseFactInfo& dfi, std::string spec) {
	std::vector<std::string> mapping;
	boost::split(mapping, spec, boost::is_any_of("@"));
	if (mapping.size() == 2) {
		dfi.kb_role_map[mapping[0]] = mapping[1];
	} else if (mapping.size() == 1) {
		dfi.kb_role_map[mapping[0]] = mapping[0];
	} else {
		std::stringstream errstr;
		errstr << "Profile slot '" << _unique_id << "' has badly formed role attribute";		
		throw UnexpectedInputException("ProfileSlot::addRoleMapping", errstr.str().c_str());
	}
}
	

/*! \brief Adds a fact to this ProfileSlot.  This method figures out whether 
    the new fact should be considered supporting evidence for an existing 
    hypothesis or if it should form the basis for a new hypothesis.

    \param fact A new fact that belongs in this slot type
*/
void ProfileSlot::addFact(PGFact_ptr fact, PGDatabaseManager_ptr pgdm) {
	_output_up_to_date = false;

	// We create an actual hypothesis for this, because sometimes there is work
	//  here that we don't want to repeat each time we run isEquiv()
	GenericHypothesis_ptr factHypoth = makeNewHypothesis(fact, pgdm);

	// Check to make sure this fact is really reportable
	// This can happen when a named entity doesn't actually map to the actor DB; we don't handle those.
	if (EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(factHypoth)) {
		if (empHypoth->getNamedArgument() == NameHypothesis_ptr())
			return;
	}
	if (NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(factHypoth)) {
		if (nameHypoth->getActorId() == -1)
			return;
	}

	bool DEBUG = false;

	if (DEBUG)
		std::wcout << factHypoth->getDisplayValue() << L"\n";

	// Is there an existing hypothesis for this fact's value?
	bool found_hypoth = false;
	bool is_better_version_of_existing_hypoth = false;
	std::set<GenericHypothesis_ptr> hypothesesToRemove;
    for(std::list<GenericHypothesis_ptr>::iterator it = _rawHypotheses.begin(); it != _rawHypotheses.end(); it++){
        GenericHypothesis_ptr hypoth = *it;
		if (hypoth->isEquivAndBetter(factHypoth)) {		
			if (DEBUG) {
				std::wcout << L"  Is better than: " << hypoth->getDisplayValue() << L"\n";
				BOOST_FOREACH(PGFact_ptr fact, hypoth->getSupportingFacts()) {
					if (fact->getAnswerArgument())
						std::wcout << L"    * " << fact->getAnswerArgument()->getResolvedStringValue() << L"\n";
				}
			}

			is_better_version_of_existing_hypoth = true;
			factHypoth->addSupportingHypothesis(hypoth);
			hypothesesToRemove.insert(hypoth);

			
			// we could keep going and collect more? but for now let's just break
			// we will still add factHypoth to _rawHypotheses outside this loop
			break;
		} else if (hypoth->isEquiv(factHypoth)) {
			// Fact matches this hypothesis, so strengthen this hypothesis 
			// with the given fact
			if (DEBUG) {
				std::wcout << L"  Is equal to: " << hypoth->getDisplayValue() << L"\n";
				BOOST_FOREACH(PGFact_ptr fact, hypoth->getSupportingFacts()) {
					if (fact->getAnswerArgument())
						std::wcout << L"    * " << fact->getAnswerArgument()->getResolvedStringValue() << L"\n";
				}
			}

			hypoth->addSupportingHypothesis(factHypoth);
			found_hypoth = true;
			
			// We found a matching hypothesis, so break
			break;
		}
	}

	if (!found_hypoth) {
		// Create a new hypothesis from this fact
		_rawHypotheses.push_back(factHypoth);
	}	
	if (DEBUG)
		std::wcout << L"\n";

	// Remove previous hypotheses that were subsumed under our new good one
    std::set<GenericHypothesis_ptr>::iterator iter1;
    for (iter1 = hypothesesToRemove.begin(); iter1 != hypothesesToRemove.end(); iter1++){
		std::list<GenericHypothesis_ptr>::iterator iter2;
		for (iter2 = _rawHypotheses.begin(); iter2 != _rawHypotheses.end(); iter2++) {
			if ((*iter2) == (*iter1)) {
				_rawHypotheses.erase(iter2);
				break;
			}
		}
	}
}


/*! \brief Private method that creates a new hypothesis using the given fact as
    its basis

    \param fact The fact used to formulate a new hypothesis

	\return A pointer to the new hypothesis
*/
GenericHypothesis_ptr ProfileSlot::makeNewHypothesis(PGFact_ptr fact, PGDatabaseManager_ptr pgdm) {

	if (_hypothesisType == "date")
		return boost::make_shared<DateHypothesis>(fact);
	else if (_hypothesisType == "phrase")
		return boost::make_shared<PhraseHypothesis>(fact);
	else if (_hypothesisType == "condition")
		return boost::make_shared<ConditionHypothesis>(fact);
	else if (_hypothesisType == "name")
		return boost::make_shared<NameHypothesis>(fact, shared_from_this(), pgdm);
	else if (_hypothesisType == "simple_string")
		return boost::make_shared<SimpleStringHypothesis>(fact, fact->getAnswerArgument());
	else if (_hypothesisType == "description")
		return boost::make_shared<DescriptionHypothesis>(fact, pgdm);
	else if (_hypothesisType == "employment" && _unique_id == EMPLOYER_UNIQUE_ID)
		return boost::make_shared<EmploymentHypothesis>(fact, EmploymentHypothesis::EMPLOYER, pgdm);
	else if (_hypothesisType == "employment" && (_unique_id == EMPLOYEE_UNIQUE_ID || _unique_id == LEADER_UNIQUE_ID))
		return boost::make_shared<EmploymentHypothesis>(fact, EmploymentHypothesis::EMPLOYEE, pgdm);

	// shouldn't happen if error-checking is up to date in constructor
	return boost::make_shared<SimpleStringHypothesis>(fact, fact->getAnswerArgument());
}

/*! \brief Determines whether this ProfileSlot contains similar (overlapping) 
    data as another ProfileSlot

    \param that the other ProfileSlot to compare against
    \return true if this ProfileSlot is similar to the other, false otherwise
*/
bool ProfileSlot::isSimilar(ProfileSlot_ptr that) {
	if (_unique_id != that->_unique_id)
		return false;

	// Generate slots that will be output, don't bother with existing profile parameter
	generateOutputHypotheses(Profile_ptr(), false);
	that->generateOutputHypotheses(Profile_ptr(), false);

	// Returns true if there is at least one hypothesis match among the output hypotheses
	BOOST_FOREACH(GenericHypothesis_ptr thisHypoth, _outputHypotheses) {		
		BOOST_FOREACH(GenericHypothesis_ptr thatHypoth, that->getOutputHypotheses()) {
			if (thisHypoth->isEquiv(thatHypoth))
				return true;
		}
	}

	return false;
}

/*! \brief  Sorts the hypotheses and decides which to output.
*/
void ProfileSlot::generateOutputHypotheses(Profile_ptr existingProfile, bool print_to_screen) {

	// If we are printing to screen, we'd like to regenerate this in this context, even if it's redundant...
	if (_output_up_to_date && !print_to_screen)
		return;

	_outputHypotheses.clear();
	_namesAlreadyDisplayed.clear();
	_titlesAlreadyDisplayed.clear();
	_nationalitiesAlreadyDisplayed.clear();

	// This is a special case that should only occur for ST_DESCRIPTION,
	//   which we want to make non-redundant with the other slots
	if (_unique_id == DESCRIPTION_UNIQUE_ID) {
		DescriptionHypothesis::gatherExistingProfileInformation(existingProfile, _namesAlreadyDisplayed,
			_titlesAlreadyDisplayed, _nationalitiesAlreadyDisplayed);
	}

	if (_rawHypotheses.empty())
		return;
	
	if (print_to_screen) {
		SessionLogger::info("PG", "wcout") << "<h2>SLOT: " << _displayName << "</h2>\n";
	}

	std::list<GenericHypothesis_ptr> unusedHypotheses;
	int n_english_hypotheses = 0;
	BOOST_FOREACH (GenericHypothesis_ptr hypoth, _rawHypotheses) {
		unusedHypotheses.push_back(hypoth);
		if (hypoth->nEnglishFacts() > 0)
			n_english_hypotheses++;
	}

	int n_foreign_hypotheses_uploaded = 0;
	while (true) {
		// Get the next best hypothesis
		GenericHypothesis_ptr bestHypothesis = getBestHypothesis(unusedHypotheses);

		// Decide whether to finish output
		if (bestHypothesis == GenericHypothesis_ptr())
			break;

		if (reachesCutoff(bestHypothesis)) 
			break;

		removeHypothesis(bestHypothesis, unusedHypotheses);

		if (!isValidHypothesis(bestHypothesis, existingProfile, n_foreign_hypotheses_uploaded, false, print_to_screen))
			continue;

		if (print_to_screen) {
			printHypothesisToScreen(existingProfile, "UPLOADED", bestHypothesis);
		}

		_outputHypotheses.push_back(bestHypothesis);		
		if (bestHypothesis->nEnglishFacts() == 0)
			n_foreign_hypotheses_uploaded++;
	}

	// Second round of hypothesis finding. Cannot be incorporated in the above 
	// while loop since the ordering isn't entirely dependent upon hypothesis
	// ranking.
	if (_unique_id == EMPLOYER_UNIQUE_ID && _outputHypotheses.size() > 0) {
		// If missing PAST employer, look to see if best PAST employer is valid
		bool has_current = false;
		bool has_past = false;
		BOOST_FOREACH (GenericHypothesis_ptr hypoth, _outputHypotheses) {
			EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypoth);
			if (empHypoth == EmploymentHypothesis_ptr()) continue; // should never happen?
			if (empHypoth->getTense() == EmploymentHypothesis::CURRENT) has_current = true;
			if (empHypoth->getTense() == EmploymentHypothesis::PAST) has_past = true;
		}

		EmploymentHypothesis::status_tense_t tense_to_find = EmploymentHypothesis::UNSET;
		if (has_current && !has_past) tense_to_find = EmploymentHypothesis::PAST;

		GenericHypothesis_ptr bestHypothesis = GenericHypothesis_ptr();
		if (tense_to_find != EmploymentHypothesis::UNSET)
			bestHypothesis = getBestEmploymentHypothesis(unusedHypotheses, tense_to_find);

		if (bestHypothesis != GenericHypothesis_ptr() && 
			bestHypothesis->nSupportingFacts() > 1 &&
			_outputHypotheses.front()->nDoublyReliableEnglishTextFacts() > 0 &&
			(float)bestHypothesis->nDoublyReliableEnglishTextFacts() / _outputHypotheses.front()->nDoublyReliableEnglishTextFacts() > 0.1 &&
			isValidHypothesis(bestHypothesis, existingProfile, n_foreign_hypotheses_uploaded, true, false))
		{
			//std::cerr << "Outputting PAST employment hypothesis for: " << UnicodeUtil::toUTF8StdString(existingProfile->getName()) << " (" << bestHypothesis->nSupportingFacts() << ")\n";;

			if (print_to_screen)
				printHypothesisToScreen(existingProfile, "UPLOADED", bestHypothesis);

			_outputHypotheses.push_back(bestHypothesis);

			removeHypothesis(bestHypothesis, unusedHypotheses);
		}

		// If title is something like "member", or organization is a political party, 
		// output it if we have enough confidence in it. Removes any hypothesis it
		// looks at from unusedHypotheses
		while (true) {
			// Get the next best hypothesis
			GenericHypothesis_ptr bestHypothesis = getBestHypothesis(unusedHypotheses);

			if (bestHypothesis == GenericHypothesis_ptr())
				break;

			if (_outputHypotheses.size() >= 4) 
				break;

			removeHypothesis(bestHypothesis, unusedHypotheses);
			
			if (!isValidHypothesis(bestHypothesis, existingProfile, n_foreign_hypotheses_uploaded, true, print_to_screen))
				continue;

			EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(bestHypothesis);
			if (empHypoth == EmploymentHypothesis_ptr()) continue; // should never happen?

			if (bestHypothesis->nDoublyReliableEnglishTextFacts() > 0 &&
				(float)bestHypothesis->nDoublyReliableEnglishTextFacts() / _outputHypotheses.front()->nDoublyReliableEnglishTextFacts() > 0.1 &&
				(empHypoth->hasSecondaryTitleWord() || empHypoth->employerIsPoliticalParty()))
			{
				//std::cerr << "Outputting secondary employment hypothesis for: " << UnicodeUtil::toUTF8StdString(existingProfile->getName()) << " (" << bestHypothesis->nSupportingFacts() << ")\n";

				if (print_to_screen)
					printHypothesisToScreen(existingProfile, "UPLOADED", bestHypothesis);
				
				_outputHypotheses.push_back(bestHypothesis);
			} else {
				if (print_to_screen)
					printHypothesisToScreen(existingProfile, "CUTOFF", bestHypothesis);
			}
		}
	}

	if (print_to_screen) {
		GenericHypothesis_ptr bestHypothesis = getBestHypothesis(unusedHypotheses);

		if (bestHypothesis != GenericHypothesis_ptr()) {
			printHypothesisToScreen(existingProfile, "CUTOFF", bestHypothesis);
			removeHypothesis(bestHypothesis, unusedHypotheses);
		}

		if (_print_all_hypotheses_to_html) {
			for (int i = 0; i < 50; i++) { // don't print more than 50 to screen
				GenericHypothesis_ptr bestHypothesis = getBestHypothesis(unusedHypotheses);
				if (bestHypothesis == GenericHypothesis_ptr()) {
					break;
				} else {
					printHypothesisToScreen(existingProfile, "CUTOFF", bestHypothesis);
					removeHypothesis(bestHypothesis, unusedHypotheses);
				}
			}
		}

		int leftovers = static_cast<int>(unusedHypotheses.size());
		bool singular = leftovers == 1;
		const char * toBe = singular ? "is" : "are";
		const char * ending = singular ? "is" : "es";
		SessionLogger::info("PG", "cout") << "<i>There " << toBe << " " << leftovers << " additional hypothes" << ending << "</i>\n";
		SessionLogger::info("PG", "cout") << "<p>\n";
	}

	_output_up_to_date = true;
}

bool ProfileSlot::isValidHypothesis(GenericHypothesis_ptr bestHypothesis, Profile_ptr existingProfile, int n_foreign_hypotheses_uploaded, bool secondary_hypothesis, bool print_to_screen) {

	std::string rationale = "";

	// We allow these slots if the support facts reach some proportion of total facts of profile target
	if (_unique_id == NATIONALITY_UNIQUE_ID || (_unique_id == EMPLOYER_UNIQUE_ID && !secondary_hypothesis)) {
		// we care how many total facts are associated with a profile; flag says count all types
		int n_raw_facts = existingProfile->countRawFacts();
		
		int n_slot_facts = bestHypothesis->nSupportingFacts();
		float slot_facts_ratio = ((float)n_slot_facts) / n_raw_facts;
		//SessionLogger::info("PG") << L"Deciding " << _displayName <<" facts=" << n_slot_facts << L" out of total facts=" << n_raw_facts << std::endl;
		if (n_raw_facts > SMALL_PROFILE_FACTS_COUNT && slot_facts_ratio < _min_facts_ratio) {
			std::stringstream rationale;
			rationale << "slot facts too few for total facts; #facts = " << n_slot_facts << ", #total = " << n_raw_facts << ", ratio = " << slot_facts_ratio << ")";
			if (print_to_screen) 
				printHypothesisToScreen(existingProfile, rationale.str(), bestHypothesis);
			return false;
		}
	}	

	if (_unique_id == DESCRIPTION_UNIQUE_ID) {				
		DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(bestHypothesis);
		if (descHypoth != DescriptionHypothesis_ptr() && 
			existingProfile != Profile_ptr() &&
			descHypoth->isSpecialIllegalHypothesis(
				rationale, _namesAlreadyDisplayed, 
				_titlesAlreadyDisplayed, _nationalitiesAlreadyDisplayed,
				existingProfile->getProfileNameHypothesis()->getGender()))
		{
			if (print_to_screen)
				printHypothesisToScreen(existingProfile, rationale, bestHypothesis);
			return false;
		}
	}

		
	if (NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(bestHypothesis)) {
		if (existingProfile != Profile_ptr() && nameHypoth->matchesProfileName(existingProfile)){
			std::wstring nameHypName = nameHypoth->getDisplayValue();
			if (print_to_screen)
				printHypothesisToScreen(existingProfile, "TOO SIMILAR TO PROFILE NAME", bestHypothesis);
			return false;
		}
	}

	if (_unique_id == DESCRIPTION_UNIQUE_ID) {
		DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(bestHypothesis);
		if (descHypoth != DescriptionHypothesis_ptr() && existingProfile != Profile_ptr()) {
			bool matched = false;
            for(size_t i = 0; i < descHypoth->getMentionArguments().size(); i++){
                NameHypothesis_ptr arg = descHypoth->getMentionArguments().at(i);
                if (arg->matchesProfileName(existingProfile)){
					matched = true;
					std::wstring nameHypName = arg->getDisplayValue();
					if (print_to_screen)
						printHypothesisToScreen(existingProfile, "Argument to descriptor matches profile name", bestHypothesis);
					break;
				}
			}
			if (matched)
				return false;
		}
	}

	// we shouldn't use phrases that have phonemic text due to non-translation
	// or which span ellisions that could alter their meaning
	if (_hypothesisType == "phrase") {
		if (existingProfile != Profile_ptr()){
			PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(bestHypothesis);
			if (phraseHypoth != PhraseHypothesis_ptr() && 
				((phraseHypoth->isPartiallyTranslated(rationale)) ||
				(phraseHypoth->isInternallyEllided(rationale)))){
					if (print_to_screen)
						printHypothesisToScreen(existingProfile, rationale, bestHypothesis);
					return false;
			}
		}
		// Only print out at most five foreign quotations/actions/sentiments
		/*if (n_foreign_hypotheses_uploaded > 5 && bestHypothesis->nEnglishFacts() == 0) {
			if (print_to_screen)
				printHypothesisToScreen(existingProfile, "foreign CUTOFF", bestHypothesis);
			return false;
		}*/
	}
	// special case to deal with "activities" which may overlap other phrase answers
	if (_unique_id == ACTIONS_UNIQUE_ID) {
		if (existingProfile != Profile_ptr()){
			std::vector<PhraseHypothesis_ptr> usedPhrases = PhraseHypothesis::existingProfilePhrases(existingProfile);

			PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(bestHypothesis);
			if (phraseHypoth != PhraseHypothesis_ptr() && 
				(phraseHypoth->isSpecialIllegalHypothesis(usedPhrases, rationale))) {
					if (print_to_screen)
						printHypothesisToScreen(existingProfile, rationale, bestHypothesis);
					return false;
			}
		}
	}

	if (isIllegalHypothesis(bestHypothesis, rationale)) {
		if (print_to_screen) 
			printHypothesisToScreen(existingProfile, rationale, bestHypothesis);
		return false;
	}

	// Make sure we're not too similar to a previous hypothesis
	bool is_similar_to_better_hypothesis = false;
	std::list<GenericHypothesis_ptr>::iterator iter;
    for(iter = _outputHypotheses.begin(); iter != _outputHypotheses.end(); iter++){
		GenericHypothesis_ptr hypoth = *iter;
        // Note: these functions are NOT symmetric due to equivalent name stuff, so make sure you call it both ways!
		if (bestHypothesis->isSimilar(hypoth) || hypoth->isSimilar(bestHypothesis)) {
			is_similar_to_better_hypothesis = true;
			break;
		}
	}

	if (is_similar_to_better_hypothesis) {
		if (print_to_screen)
			printHypothesisToScreen(existingProfile, "TOO SIMILAR TO BETTER", bestHypothesis);
		return false;
	}

	if (_unique_id == EMPLOYEE_UNIQUE_ID) {
		// Make sure we're not too similar to a previous LEADER hypothesis (this should already have been generated,
		//   since it would always appear prior to this slot in display order)
		bool is_similar_to_better_leader_hypothesis = false;
		ProfileSlot_ptr leaderSlot = existingProfile->getExistingSlot(LEADER_UNIQUE_ID);
		if (leaderSlot != ProfileSlot_ptr()) {
	        std::list<GenericHypothesis_ptr>::iterator iter;
            for(iter = leaderSlot->getOutputHypotheses().begin(); iter != leaderSlot->getOutputHypotheses().end(); iter++){
		        GenericHypothesis_ptr hypoth = *iter;
				// Note: these functions are NOT symmetric due to equivalent name stuff, so make sure you call it both ways!
				if (bestHypothesis->isSimilar(hypoth) || hypoth->isSimilar(bestHypothesis)) {
					is_similar_to_better_leader_hypothesis = true;
					break;
				}
			}
		}
		if (is_similar_to_better_leader_hypothesis) {
			if (print_to_screen)
				printHypothesisToScreen(existingProfile, "TOO SIMILAR TO BETTER (LEADER)", bestHypothesis);
			return false;
		}
	}

	return true;
}

GenericHypothesis_ptr ProfileSlot::getBestHypothesis(std::list<GenericHypothesis_ptr>& unusedHypotheses) {
	GenericHypothesis_ptr bestHypothesis = GenericHypothesis_ptr();
	BOOST_FOREACH (GenericHypothesis_ptr hypoth, unusedHypotheses) {
		if (bestHypothesis == GenericHypothesis_ptr())
			bestHypothesis = hypoth;
		else if (hypoth->rankAgainst(bestHypothesis) == GenericHypothesis::BETTER) {
			bestHypothesis = hypoth;
		}
	}
	return bestHypothesis;
}

GenericHypothesis_ptr ProfileSlot::getBestEmploymentHypothesis(std::list<GenericHypothesis_ptr>& unusedHypotheses, int status) {
	GenericHypothesis_ptr bestHypothesis = GenericHypothesis_ptr();

	BOOST_FOREACH (GenericHypothesis_ptr hypoth, unusedHypotheses) {
		EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypoth);
		if (empHypoth == EmploymentHypothesis_ptr()) continue;
		if (empHypoth->getTense() != status) continue;

		if (bestHypothesis == GenericHypothesis_ptr())
			bestHypothesis = hypoth;
		else if (hypoth->rankAgainst(bestHypothesis) == GenericHypothesis::BETTER) {
			bestHypothesis = hypoth;
		}
	}
	return bestHypothesis;
}

void ProfileSlot::removeHypothesis(GenericHypothesis_ptr hypo, std::list<GenericHypothesis_ptr>& hypotheses) {
	for (std::list<GenericHypothesis_ptr>::iterator iter = hypotheses.begin(); iter != hypotheses.end(); ++iter) {
		if (hypo == (*iter)) {
			hypotheses.erase(iter);
			break;
		}
	}
}

bool ProfileSlot::reachesCutoff(GenericHypothesis_ptr hypo) {
	if (_max_uploaded_hypoth >= 0 && (int) _outputHypotheses.size() >= _max_uploaded_hypoth)
		return true;

	if (_unique_id == NATIONALITY_UNIQUE_ID || _unique_id == EMPLOYER_UNIQUE_ID) {
		// We allow two only if both are very strong
		if (_outputHypotheses.size() > 0) {
			if (hypo->nReliableEnglishTextFacts() < 10)
				return true;
			float relative_support = (float)hypo->nReliableEnglishTextFacts() / _outputHypotheses.front()->nReliableEnglishTextFacts();
			if (relative_support < 0.3F)
				return true;
			return false;
		}
	}

	return false;
}

bool ProfileSlot::isIllegalHypothesis(GenericHypothesis_ptr hypo, std::string& rationale) {

	if (hypo->isIllegalHypothesis(shared_from_this(), rationale))
		return true;

	// Once we've gotten some good ones, criteria get stricter
	if (_outputHypotheses.size() >= 5) {
		if (hypo->isRiskyHypothesis(shared_from_this(), rationale))
			return true;
	}
	
	return false;
}


void ProfileSlot::printHypothesisToScreen(Profile_ptr existingProfile, std::string status, GenericHypothesis_ptr hypo) {

	std::wstring wdisplayValue = hypo->getDisplayValue();

	if (DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypo)) {
		wdisplayValue = existingProfile->stripOfResolutionsToThisActor(wdisplayValue);
	}

	// Sadly, wcout will die if it gets non-ascii. So, we remove them for display purposes.	
	std::string displayValue = UnicodeUtil::toUTF8StdString(wdisplayValue);	

	//if (status == "unreliable descriptor")
	//	return;
    std::ostringstream ostr;
	if (status == "UPLOADED") {
		ostr << "<b>[uploaded] " << displayValue << "</b>";
		existingProfile->printDebugInfoToFile(_displayName + "\t");
		existingProfile->printDebugInfoToFile(wdisplayValue + L"\t");
	} else {
		ostr << "<font color=gray>" << displayValue << " (" << status << ")";
	}
	ostr << "<br>\n";


	int max_facts_to_print = 20;
	DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypo);
	if (descHypoth != DescriptionHypothesis_ptr()) {
		BOOST_FOREACH(NameHypothesis_ptr name, descHypoth->getMentionArguments()) {
			ostr << "Name: " << UnicodeUtil::toUTF8StdString(name->getDisplayValue()) << "<br>";
		}
		BOOST_FOREACH(SimpleStringHypothesis_ptr name, descHypoth->getNonMentionArguments()) {
			ostr << "Non-name: " << UnicodeUtil::toUTF8StdString(name->getDisplayValue()) << "<br>";
		}
		ostr << "Headword: " << UnicodeUtil::toUTF8StdString(descHypoth->getHeadword()) << "<br>";
        PGFact_ptr fact;
        for (int i = 0; i < std::min<int>(static_cast<int>(descHypoth->getSupportingFacts().size()), max_facts_to_print); i++){
			fact = descHypoth->getSupportingFacts().at(i);
			PGFactArgument_ptr extentArg = fact->getFirstArgumentForRole("Extent");
			if (extentArg)
				ostr << "Fact: " << extentArg->getLiteralStringValue() << "<br>";
		}			
		if (descHypoth->isCopula())
			ostr << "Copula<br>";
		if (descHypoth->isAppositive())
			ostr << "Appositive<br>";
		ostr << "NSupporting: " << descHypoth->nSupportingFacts() << "<br>";
		ostr << "NModifiers: " << descHypoth->getNModifiers() << "<br>";
	}	
	PhraseHypothesis_ptr phraseHypoth = boost::dynamic_pointer_cast<PhraseHypothesis>(hypo);
	if (phraseHypoth != PhraseHypothesis_ptr()) {
		ostr << "Phrase: " << phraseHypoth->nReliableEnglishTextFacts() << " " << phraseHypoth->nEnglishFacts() << " " << phraseHypoth->nReliableFacts() << " " << phraseHypoth->getNewestCaptureTime() << "\n";
	}	
	ostr << "Oldest fact ID: " << fixed << setprecision(0) << hypo->getOldestFactID() << "<br>";
	ostr << "Newest fact ID: " << fixed << setprecision(0) << hypo->getNewestFactID() << "<br>";
	ostr << "Oldest capture time: " << hypo->getOldestCaptureTime()  << "<br>";
	ostr << "Newest capture time: " << hypo->getNewestCaptureTime()  << "<br>";
	
	std::string segment_root = ParamReader::getParam("prototype_experiment_directory");	
	int epoch_id = ParamReader::getOptionalIntParamWithDefaultValue("epoch", 100);

	PGFact_ptr fact;

	PGFact_ptr bestSupportingFact;
	if (existingProfile->debugIsOn() && status == "UPLOADED") {
		// Inefficient, for debugging only!
		for (int i = 0; i < std::min<int>(static_cast<int>(hypo->getSupportingFacts().size()), max_facts_to_print); i++){
			fact = hypo->getSupportingFacts().at(i);
			if (bestSupportingFact == PGFact_ptr()) {
				bestSupportingFact = fact;
				continue;
			}
			// Always prefer English
			if (bestSupportingFact->isMT() && !fact->isMT()) {
				bestSupportingFact = fact;
				continue;
			} else if (fact->isMT() && !bestSupportingFact->isMT()) {
				continue;
			}

			// Then always prefer doubly-reliable
			bool fact_is_double = fact->hasReliableAgentMentionConfidence() && fact->hasReliableAnswerMentionConfidence();
			bool best_fact_is_double = bestSupportingFact->hasReliableAgentMentionConfidence() && bestSupportingFact->hasReliableAnswerMentionConfidence();
			if (!best_fact_is_double && fact_is_double) {
				bestSupportingFact = fact;
				continue;
			} else if (best_fact_is_double && !fact_is_double) {
				continue;
			}

			if (fact->getScoreGroup() != -1 && bestSupportingFact->getScoreGroup() != -1) {
				if (fact->getScoreGroup() < bestSupportingFact->getScoreGroup()) {
					bestSupportingFact = fact;
					continue;
				} else if (fact->getScoreGroup() > bestSupportingFact->getScoreGroup()) {
					continue;
				}
			}

			if (fact->getScore() > bestSupportingFact->getScore()) {
				bestSupportingFact = fact;
				continue;
			} else if (fact->getScore() < bestSupportingFact->getScore()) {
				continue;
			}

			if (fact->getFactId() < bestSupportingFact->getFactId()) {
				bestSupportingFact = fact;
				continue;
			} 

		}
		std::stringstream fbuf;
		if (bestSupportingFact->getAnswerArgument())
			fbuf << fact->getAnswerArgument()->getLiteralStringValue() << " ";
		if (bestSupportingFact->hasReliableAgentMentionConfidence() && bestSupportingFact->hasReliableAnswerMentionConfidence())
			fbuf << "   doubly-reliable ";
		else if (bestSupportingFact->hasReliableAnswerMentionConfidence())
			fbuf << "   answer-reliable ";
		else if (bestSupportingFact->hasReliableAgentMentionConfidence())
			fbuf << "   agent-reliable ";
		if (bestSupportingFact->isMT())
			fbuf << "MT ";
		fbuf << "support=" << bestSupportingFact->getSupportChunkValue() << " ";
		existingProfile->printDebugInfoToFile(fbuf.str() + "\n");
	}	

    for (int i = 0; i < std::min<int>(static_cast<int>(hypo->getSupportingFacts().size()), max_facts_to_print); i++){
		fact = hypo->getSupportingFacts().at(i);
		std::stringstream fbuf;
		if (fact->getAnswerArgument())
			fbuf << fact->getAnswerArgument()->getLiteralStringValue() << " ";
		if (fact->hasReliableAgentMentionConfidence() && fact->hasReliableAnswerMentionConfidence())
			fbuf << "   doubly-reliable ";
		else if (fact->hasReliableAnswerMentionConfidence())
			fbuf << "   answer-reliable ";
		else if (fact->hasReliableAgentMentionConfidence())
			fbuf << "   agent-reliable ";
		if (fact->getScoreGroup() != -1)
			fbuf << "   score-group=" << fact->getScoreGroup() << " ";
		if (fact->isMT())
			fbuf << "MT ";
		fbuf << "docid=" << fact->getDocumentId() << " ";
		fbuf << "support=" << fact->getSupportChunkValue() << " ";
		ostr << "<i>Fact:" << fbuf.str() << "</i><br>\n";
	}

	printDate(ostr, "START", hypo->getStartDate());
	printDate(ostr, "END", hypo->getEndDate());
	printDates(ostr, "HOLD", hypo->getHoldDates());
	printDates(ostr, "ACTIVITY", hypo->getActivityDates());
	printDates(ostr, "NON_HOLD", hypo->getNonHoldDates());

	if (status.compare("UPLOADED") != 0)
		ostr << "</font>";
	ostr << "<br>\n";

    SessionLogger::info("PG", "wcout") << ostr.str();
}

void ProfileSlot::printDate(std::ostringstream& ostr, std::string header, PGFactDate_ptr date) {
	if (date != PGFactDate_ptr()) {
		ostr << header << ":&nbsp;&nbsp;" << date->getDBString() << "<br>\n";
	}
}

void ProfileSlot::printDates(std::ostringstream& ostr, std::string header, std::vector<PGFactDate_ptr>& dates) {
	if (dates.size() == 0)
		return;

	ostr << header << ":&nbsp;&nbsp;";
	BOOST_FOREACH(PGFactDate_ptr factDate, dates)
		ostr << factDate->getDBString() << "&nbsp;&nbsp;";
	ostr << "<br>\n";
}

void ProfileSlot::generateSlotDates() {
	BOOST_FOREACH(GenericHypothesis_ptr hypo, _rawHypotheses) {
		hypo->generateHypothesisDates();
		if (hypo->excludeQuestionableHoldDates()) {
			size_t num_hold_dates = hypo->getHoldDates().size();
			size_t num_non_hold_dates = hypo->getNonHoldDates().size();

			if (num_hold_dates > 0 && (float)num_hold_dates / (num_hold_dates + num_non_hold_dates) < .11 && num_hold_dates <= 2)
				hypo->clearHoldDates();
			if (num_non_hold_dates > 0 && (float)num_non_hold_dates / (num_hold_dates + num_non_hold_dates) < .11 && num_non_hold_dates <= 2) {
				hypo->clearNonHoldDates();
			}
		}
	}
}
