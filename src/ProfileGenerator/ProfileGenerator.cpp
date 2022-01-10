// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/StringTransliterator.h"
#include "Generic/common/ParamReader.h"
#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "boost/regex.hpp"
#include "boost/algorithm/string.hpp"

#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/PGActorInfo.h"
#include "ProfileGenerator/PGDatabaseManager.h"
#include "ProfileGenerator/GenericHypothesis.h"

#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileGenerator.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/ProfileConfidence.h"

#include <iostream>
#include <stdio.h>
#include <cctype>
#include <set>
#include <ctime>

#define MAX_NAME 256

const float ProfileGenerator::FREQUENCY_CUTOFF_PERCENT = 0.1f;
std::map<std::wstring, std::wstring> ProfileGenerator::_normalizedNameCache;

ProfileGenerator::ProfileGenerator(PGDatabaseManager_ptr pgdm, ProfileConfidence_ptr pc)
{
	_pgDatabaseManager = pgdm;
	_confModel = pc;	

	// num_profile_generator_retries specifies the number of times to retry a profile before giving up
	_num_retries = ParamReader::getOptionalIntParamWithDefaultValue("num_profile_generator_retries", 0);
	
	// write_html = true causes html for debugging, false is quiet
	_write_profile_to_html = ParamReader::getOptionalTrueFalseParamWithDefaultVal("write_html", false);

	using namespace SerifXML;

	_profileSlotDocument = XMLUtil::loadXercesDOMFromFilename(ParamReader::getRequiredParam("profile_slot_types").c_str());
	xercesc::DOMNodeList* slots = _profileSlotDocument->getElementsByTagName(X_slot);
	for (size_t i = 0; i < slots->getLength(); i++) {
		xercesc::DOMElement* slot = dynamic_cast<xercesc::DOMElement*>(slots->item(i));
		if (slot->hasAttribute(X_unique_id)) {
			std::string unique_id = SerifXML::transcodeToStdString(slot->getAttribute(X_unique_id));
			if (_profileSlotElements.find(unique_id) != _profileSlotElements.end()) {
				std::stringstream errstr;
				errstr << "Two profile slots with the same unique_id: " << unique_id;
				throw UnexpectedInputException("ProfileGenerator::ProfileGenerator", errstr.str().c_str());
			}
			std::stringstream rankStr;
			rankStr << i;
			slot->setAttribute(X_rank, xercesc::XMLString::transcode(rankStr.str().c_str())); // rank is determined by position in the XML file
			_profileSlotElements[unique_id] = slot;
		} else {
			throw UnexpectedInputException("ProfileGenerator::ProfileGenerator", "Profile slot without unique id");
		}		
	}
}

ProfileGenerator::~ProfileGenerator() {
	_profileSlotDocument->release();
}

bool ProfileGenerator::uploadProfile(Profile_ptr prof) {

	for (int i = 0; i < _num_retries + 1; i++) {
		try {
			_pgDatabaseManager->uploadProfile(prof, _confModel, _write_profile_to_html);
			return true;
		} catch (...) {
			if (i == _num_retries) 
				throw;
		}
	}
	
	// Shouldn't get here	
	return false;
}

Profile_ptr ProfileGenerator::generateFullProfile(std::wstring entityName, Profile::profile_type_t profileType) {

	PGActorInfo_ptr target_actor = _pgDatabaseManager->getBestActorForName(entityName, profileType);
	
	if (target_actor == PGActorInfo_ptr()) {
		if (_write_profile_to_html)
			SessionLogger::info("PG", "wcout") << L"<h2>No database actor for " << entityName << "</h2>\n";
		return Profile_ptr();
	}

	if (_write_profile_to_html)
		SessionLogger::info("PG", "wcout") << L"<h2>User requested " << entityName << "; id " << target_actor->getActorId() << " found</h2>\n";

	return generateFullProfile(target_actor);
}

Profile_ptr ProfileGenerator::generateFullProfile(PGActorInfo_ptr target_actor) {

	if (target_actor == PGActorInfo_ptr()) {
		// no profile for this string
		return Profile_ptr();
	}
	
	if (_write_profile_to_html)
		SessionLogger::info("PG", "wcout") << L"<h2> Profiling " << target_actor->getCanonicalActorName() << "</h2>\n";

	Profile_ptr prof = boost::make_shared<Profile>(target_actor->getActorId(), target_actor->getCanonicalActorName(),
		Profile::getProfileTypeForString(target_actor->getEntityType()), _pgDatabaseManager);

	bool success = false;
	for (int i = 0; i < _num_retries + 1; i++) {
		try {
			populateProfile(prof);
			return prof;
		} catch (...) {
			if (i == _num_retries) 
				throw;
		}
	}
	
	// Shouldn't get here
	return Profile_ptr();
}

void ProfileGenerator::populateProfile(Profile_ptr prof) {

	// We don't want these to get too big, so we clear it every time we populate a profile
	_pgDatabaseManager->clearActorStringCache();
	_normalizedNameCache.clear();
	
	for (std::map<std::string, xercesc::DOMElement*>::const_iterator iter = _profileSlotElements.begin(); iter != _profileSlotElements.end(); ++iter) {
		xercesc::DOMElement* slot = (*iter).second;
		if (slot->hasAttribute(SerifXML::X_target_profile)) {
			std::string target_profile = SerifXML::transcodeToStdString(slot->getAttribute(SerifXML::X_target_profile));
			if (prof->getProfileType() == Profile::ORG && target_profile.find("org") != std::string::npos)
				prof->addSlot(slot, _pgDatabaseManager);
			else if (prof->getProfileType() == Profile::PER && target_profile.find("per") != std::string::npos)
				prof->addSlot(slot, _pgDatabaseManager);
			else if (prof->getProfileType() == Profile::GPE && target_profile.find("gpe") != std::string::npos)
				prof->addSlot(slot, _pgDatabaseManager);
			else if (prof->getProfileType() == Profile::PSN && target_profile.find("psn") != std::string::npos)
				prof->addSlot(slot, _pgDatabaseManager);
		}
	}

	std::vector<int> actor_ids;
	std::vector<int> actor_ids_including_spokespeople;

	actor_ids.push_back(prof->getActorId());
	actor_ids_including_spokespeople.push_back(prof->getActorId());
	
	std::vector<int> targetFactIds;
	std::vector<int> targetFactIdsWithSpokespeople;

	for (ProfileSlotMap::const_iterator iter = prof->getSlots().begin(); iter != prof->getSlots().end(); ++iter) {
		ProfileSlot_ptr slot = (*iter).second;
		BOOST_FOREACH(ProfileSlot::DatabaseFactInfo dfi, slot->getDatabaseFactInfoList()) {
			if (dfi.fact_type != -1) {
				targetFactIds.push_back(dfi.fact_type);
				if (slot->useSpokespeople() && (prof->getProfileType() == Profile::ORG || prof->getProfileType() == Profile::GPE))
					targetFactIdsWithSpokespeople.push_back(dfi.fact_type);
			}
		}
	}

	std::vector<PGFact_ptr> allFacts = _pgDatabaseManager->getFacts(actor_ids, targetFactIds);
	std::vector<PGFact_ptr> allFactsWithSpokespeople;
	
	// Special case: For organizations, we first have to find spokespeople and add them to our lists
	// We also want to set the 'reliable' bit on the actual employee/leader hypotheses so we can use
	//  it for our profile generation
	if (prof->getProfileType() == Profile::ORG || prof->getProfileType() == Profile::GPE) {

		// TODO_LATER: figure out what the right threshold is here
		std::set<int> reliableEmployeeIds = _pgDatabaseManager->getReliableEmployees(prof->getActorId());

		// Note: Both ST_EMPLOYEE and ST_TOP_BRASS slots will contain all employees!
		//       It's only at output-time that they get filtered to contain only low-level/high-level employees (respectively)
		ProfileSlot_ptr employeeSlot = prof->getExistingSlot(ProfileSlot::EMPLOYEE_UNIQUE_ID);
		ProfileSlot_ptr leaderSlot = prof->getExistingSlot(ProfileSlot::LEADER_UNIQUE_ID);
		generateSlot(prof, employeeSlot, allFacts);
		generateSlot(prof, leaderSlot, allFacts);

		if (employeeSlot != ProfileSlot_ptr()) {
			for(std::list<GenericHypothesis_ptr>::const_iterator iter = employeeSlot->getRawHypotheses().begin(); iter != employeeSlot->getRawHypotheses().end(); ++iter) {
				EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(*iter);
				if (empHypoth == EmploymentHypothesis_ptr() || !empHypoth->isEmployeeHypothesis()) 
					continue; // should never happen
				NameHypothesis_ptr hypEmployee = empHypoth->getNamedArgument();
				if (hypEmployee == NameHypothesis_ptr())
					continue; // should never happen
				if (reliableEmployeeIds.find(hypEmployee->getActorId()) != reliableEmployeeIds.end())
					empHypoth->setReliableFromEmployeePOV(true);
				else continue;
				std::string dontCareWhy = "";
				if (empHypoth->isIllegalHypothesis(employeeSlot, dontCareWhy) && empHypoth->isIllegalHypothesis(leaderSlot, dontCareWhy))
					continue;
				if (empHypoth->isSpokesman())
					actor_ids_including_spokespeople.push_back(hypEmployee->getActorId());
			}
		}

		// Still need to set the reliable bit for the TOP BRASS slots-- they are different C++ objects
		if (leaderSlot != ProfileSlot_ptr()) {
			for(std::list<GenericHypothesis_ptr>::const_iterator iter = leaderSlot->getRawHypotheses().begin(); iter != leaderSlot->getRawHypotheses().end(); ++iter) {
				EmploymentHypothesis_ptr empHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(*iter);
				if (empHypoth == EmploymentHypothesis_ptr() || !empHypoth->isEmployeeHypothesis()) 
					continue; // should never happen
				NameHypothesis_ptr hypEmployee = empHypoth->getNamedArgument();
				if (hypEmployee == NameHypothesis_ptr())
					continue; // should never happen
				if (reliableEmployeeIds.find(hypEmployee->getActorId()) != reliableEmployeeIds.end())
					empHypoth->setReliableFromEmployeePOV(true);
				else continue;				
			}
		}

		if (targetFactIdsWithSpokespeople.size() != 0)
			allFactsWithSpokespeople = _pgDatabaseManager->getFacts(actor_ids_including_spokespeople, targetFactIdsWithSpokespeople);
	}

	// Uncomment this statement to avoid segmentation fault in Linux Release mode
	//std::cerr << "Generating slots\n";
	//std::cerr.flush();

	for (ProfileSlotMap::const_iterator iter = prof->getSlots().begin(); iter != prof->getSlots().end(); ++iter) {
		ProfileSlot_ptr slot = (*iter).second;
		if (slot->useSpokespeople())
			generateSlot(prof, slot, allFactsWithSpokespeople);
		else generateSlot(prof, slot, allFacts);
	}

	return;
}

/*! \brief Builds a slot in the given profile using the supplied facts
*/
void ProfileGenerator::generateSlot(Profile_ptr prof, ProfileSlot_ptr slot, std::vector<PGFact_ptr>& allFacts)
{
	if (slot == ProfileSlot_ptr())
		return;

	// Uncomment this statement to avoid segmentation fault in Linux Release mode
	//std::cerr << slot->getUniqueId() << "\n";
	//std::cerr.flush();

	time_t start = time(NULL);

	std::vector<PGFact_ptr> facts;
	BOOST_FOREACH(PGFact_ptr fact, allFacts) {
		BOOST_FOREACH(ProfileSlot::DatabaseFactInfo dfi, slot->getDatabaseFactInfoList()) {
			// We copy the fact and transform its roles in accordance with the DFI spec
			if (fact->matchesDFI(dfi))
				facts.push_back(boost::make_shared<PGFact>(fact, dfi));
		}
	}
	if (!facts.empty()) {		
		BOOST_FOREACH(PGFact_ptr fact, facts) {
			slot->addFact(fact, _pgDatabaseManager);
		}
		slot->generateSlotDates();
	}
	
	time_t end = time(NULL);
	time_t elapsed = end - start;
	//std::cerr << elapsed << " seconds for " << slot->getUniqueId() << " generation with " << facts.size() << " facts\n";
}

std::wstring ProfileGenerator::normalizeName(std::wstring name) {
	std::map<std::wstring, std::wstring>::iterator iter = _normalizedNameCache.find(name);
	if (iter != _normalizedNameCache.end())
		return (*iter).second;

	std::wstring normalizedName =  UnicodeUtil::normalizeTextString(name);	
	normalizedName = UnicodeUtil::normalizeNameString(normalizedName);
	// because empty strings can cause DB crashes elsewhere
	if (normalizedName == L"")
		normalizedName = L"NO_NAME";

	// Because, seriously y'all, this is crazy, and we never get all of these looking
	//   at full names in the corpus, the cross-product is too big
	boost::replace_all(normalizedName, L"mohammed", L"muhammad");
	boost::replace_all(normalizedName, L"mohammad", L"muhammad");
	boost::replace_all(normalizedName, L"muhammed", L"muhammad");
	boost::replace_all(normalizedName, L"muhamed", L"muhammad");
	boost::replace_all(normalizedName, L"muhamad", L"muhammad");
	boost::replace_all(normalizedName, L"mohamad", L"muhammad");
	boost::replace_all(normalizedName, L"mohamed", L"muhammad");
	boost::replace_all(normalizedName, L"abdel", L"abdul");

	_normalizedNameCache[name] = normalizedName;
	return normalizedName;
}

std::wstring ProfileGenerator::tokenizeName(std::wstring name) {
	std::wstring tokenizedName = name;
	boost::replace_all(tokenizedName, L"'", " '");
	boost::replace_all(tokenizedName, L"-", " - ");
	boost::replace_all(tokenizedName, L"  ", " ");

	// because empty strings can cause DB crashes elsewhere
	if (tokenizedName == L"")
		tokenizedName = L"NO_NAME";

	return tokenizedName;
}
