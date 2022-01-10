// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"

#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/ProfileConfidence.h"
#include "ProfileGenerator/TeaserBiography.h"
#include "ProfileGenerator/PGDatabaseManager.h"

#include "boost/foreach.hpp"

#include <sstream>
#include <iostream>

Profile::Profile(int actor_id, std::wstring canonicalActorName, profile_type_t profileType, PGDatabaseManager_ptr pgdm)
	: _actor_id(actor_id), _canonical_actor_name(canonicalActorName), _profile_type(profileType), _pgdm(pgdm)
{
	_profileNameHypothesis = boost::make_shared<NameHypothesis>(actor_id, canonicalActorName, _profile_type == Profile::PER, _pgdm);

	_document_canonical_names = pgdm->getDocumentCanonicalNamesForActor(actor_id);	

	// This is so we can strip descriptions of their resolutions to themselves, e.g. "the first black president [Barack Obama]"
	std::wstringstream document_canonical_name_regex_str;
	document_canonical_name_regex_str << " \\[(";
	bool begin = true;
	BOOST_FOREACH(std::wstring name, _document_canonical_names) {		
		boost::replace_all(name, L"\\", L"\\\\");
		boost::replace_all(name, L"^", L"\\^");
		boost::replace_all(name, L"$", L"\\$");
		boost::replace_all(name, L".", L"\\.");
		boost::replace_all(name, L"|", L"\\|");
		boost::replace_all(name, L"(", L"\\(");
		boost::replace_all(name, L")", L"\\)");
		boost::replace_all(name, L"[", L"\\[");
		boost::replace_all(name, L"]", L"\\]");
		boost::replace_all(name, L"{", L"\\{");
		boost::replace_all(name, L"}", L"\\}");
		boost::replace_all(name, L"*", L"\\*");
		boost::replace_all(name, L"+", L"\\+");
		boost::replace_all(name, L"?", L"\\?");
		if (!begin)
			document_canonical_name_regex_str << L"|";
		document_canonical_name_regex_str << name;
		begin = false;
	}
	document_canonical_name_regex_str << ")\\]";
	std::wstring regex_str = document_canonical_name_regex_str.str();
	
	_document_canonical_name_regex = boost::wregex(regex_str, boost::regex::icase);

	_DEBUG_ON = false;
	std::string debug_prefix = ParamReader::getParam("profile_generator_debug_directory");
	if (debug_prefix.size() > 0 && debug_prefix != "NONE") {
		_DEBUG_ON = true;
		std::stringstream filepath;
		filepath << debug_prefix << SERIF_PATH_SEP;
		if (_profile_type == PER)
			filepath << "PER-";
		else if (_profile_type == ORG)
			filepath << "ORG-";
		else if (_profile_type == GPE)
			filepath << "GPE-";
		filepath << actor_id << ".txt";
		_debugStream.open(filepath.str().c_str());
		_debugStream << L"Canonical actor name: " << _canonical_actor_name << L"\n";
	}
}

Profile::~Profile() {
	if (_DEBUG_ON)
		_debugStream.close();
}

std::wstring Profile::stripOfResolutionsToThisActor(std::wstring display_value) {
	std::wstring result = boost::regex_replace(display_value, _document_canonical_name_regex, L"");
	return result;
}

//
// Profile type management
//

std::string Profile::getStringForProfileType(profile_type_t profileType) {
	if (profileType == Profile::PER)
		return "PER";
	else if (profileType == Profile::ORG)
		return "ORG";
	else if (profileType == Profile::GPE)
		return "GPE";
	else if (profileType == Profile::PSN)
		return "PSN";
	else if (profileType == Profile::UNKNOWN)
		return "";
	else return "";
}

Profile::profile_type_t Profile::getProfileTypeForString(std::string str) {
	if (str == "PER")
		return Profile::PER;
	else if (str == "ORG")
		return Profile::ORG;
	else if (str == "GPE")
		return Profile::GPE;
	else if (str == "PSN")
		return Profile::PSN;
	else return Profile::UNKNOWN;
}

//
// Slot Management
// ---------------
// 
// NOTE: There is at most one slot object associated with each slot type, 
//   but there will almost certainly be multiple hypotheses inside 
//   one ProfileSlot object.
//

ProfileSlot_ptr Profile::getExistingSlot(std::string slot_name) {
	std::map<std::string, ProfileSlot_ptr>::const_iterator iter;
	iter = _slots.find(slot_name);
	if (iter != _slots.end()) 
		return iter->second;
	else return ProfileSlot_ptr();
}

ProfileSlot_ptr Profile::addSlot(xercesc::DOMElement* slot, PGDatabaseManager_ptr pgdm) {
	ProfileSlot_ptr profSlotPtr = boost::make_shared<ProfileSlot>(slot, pgdm);
	_slots.insert(std::pair<std::string, ProfileSlot_ptr> (profSlotPtr->getUniqueId(), profSlotPtr));
	return profSlotPtr;
}

//
// Helper function for use with evidence ratios
//
// Counts facts of all type sif the type is set to -1, else only the matching slot type
//
int Profile::countRawFacts(std::string slot_name){
	int nRawFacts = 0;
	std::pair<std::string, ProfileSlot_ptr> typeSlotPair;
	BOOST_FOREACH(typeSlotPair, _slots) {
		if (slot_name != "" && slot_name != typeSlotPair.first) 
			continue;
		std::list<GenericHypothesis_ptr> rawHypotheses = typeSlotPair.second->getRawHypotheses();
		BOOST_FOREACH(GenericHypothesis_ptr rawHypo, rawHypotheses) {
			nRawFacts += rawHypo->nSupportingFacts();
		}
	}
	return nRawFacts;
}

//
// Prepare (i.e. finalize) profile for upload (generate teaser bio, set confidences, etc.)
// 
// Prints to screen for debugging if print_to_screen is true
//
void Profile::prepareForUpload(ProfileConfidence_ptr confidenceModel, bool print_to_screen) {
	std::pair<std::string, ProfileSlot_ptr> typeSlotPair, newTypeSlotPair;

	// sort them first, solely for debugging consistency purposes
	std::list<std::pair<std::string, ProfileSlot_ptr> > sortedSlots;
	for (ProfileSlotMap::iterator iter = _slots.begin(); iter != _slots.end(); iter++) {
		newTypeSlotPair = *iter;
		bool inserted = false;
		for (std::list<std::pair<std::string,ProfileSlot_ptr> >::iterator typeSlotPairPtr = sortedSlots.begin(); typeSlotPairPtr != sortedSlots.end(); typeSlotPairPtr++) {
			if (typeSlotPairPtr->second->getRank() > newTypeSlotPair.second->getRank()) {
				sortedSlots.insert(typeSlotPairPtr, newTypeSlotPair);
				inserted = true;
				break;
			}
		}
		if (!inserted)
			sortedSlots.push_back(newTypeSlotPair);
	}

	// Generate baseline output hypotheses first for all slots except descriptions
	BOOST_FOREACH(typeSlotPair, sortedSlots) {
		if (typeSlotPair.second->getUniqueId() == ProfileSlot::DESCRIPTION_UNIQUE_ID)
			continue;
		typeSlotPair.second->generateOutputHypotheses(shared_from_this(), print_to_screen);
	}

	// For cross-slot management of the description slot, so it is not repetitive
	ProfileSlot_ptr descriptionSlot = getExistingSlot(ProfileSlot::DESCRIPTION_UNIQUE_ID);
	if (descriptionSlot != ProfileSlot_ptr()) {
		descriptionSlot->makeOutOfDate();
		descriptionSlot->generateOutputHypotheses(shared_from_this(), print_to_screen);
	}

	// Now create the teaser biography
	ProfileSlot_ptr teaserBioSlot = getExistingSlot(ProfileSlot::TEASER_BIO_UNIQUE_ID);
	if (teaserBioSlot != ProfileSlot_ptr()) {
		TeaserBiography_ptr teaser_bio = boost::make_shared<TeaserBiography>(shared_from_this());	
		std::wstring nullWString = L"";
		std::string nullString = "";
		if (teaser_bio->getBiography() != L"") {
			PGFact_ptr fact = boost::make_shared<PGFact>();
			PGFactArgument_ptr factArg = boost::make_shared<PGFactArgument>(-1, -1,
				nullString, nullWString, teaser_bio->getBiography(), teaserBioSlot->getAnswerRole(), 1.0, false);
			fact->addArgument(factArg, true);
			teaserBioSlot->addFact(fact, _pgdm);
			teaserBioSlot->generateOutputHypotheses(shared_from_this(), print_to_screen);
		}
	}

	// Last step: set confidences for hypotheses in each slot-- currently a no-op
	BOOST_FOREACH(typeSlotPair, _slots) {
		std::list<GenericHypothesis_ptr>& hypotheses = typeSlotPair.second->getOutputHypotheses();
		BOOST_FOREACH(GenericHypothesis_ptr hypothesis, hypotheses) {
			hypothesis->setConfidence(1.0);
		}
	}
}

