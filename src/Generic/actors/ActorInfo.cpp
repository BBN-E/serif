// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

// We could potentially do some caching here, if this is a bottleneck.

#include "Generic/common/leak_detection.h"

#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/actors/AWAKEActorInfo.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <iostream>

ActorInfo::ActorInfo()
{ }

ActorInfo::~ActorInfo() 
{ }

ActorInfo_ptr ActorInfo::getAppropriateActorInfoForICEWS() {
	if (ParamReader::isParamTrue("use_awake_db_for_icews"))
		return AWAKEActorInfo::getAWAKEActorInfo();
	else return ICEWSActorInfo::getICEWSActorInfo();
}

// Not implemented in AWAKE, which doesn't have codes and where actor ids = country ids

std::wstring ActorInfo::getSectorName(Symbol sectorCode) {
	throw UnrecoverableException("ActorInfo::getSectorName", "Function not defined for derived ActorInfo class");
	return L"";
}

ActorId ActorInfo::getActorByCode(Symbol actorCode) {
	throw UnrecoverableException("ActorInfo::getActorByCode", "Function not defined for derived ActorInfo class");
	return ActorId();
}

ActorId ActorInfo::getActorIdForCountry(CountryId countryId) {
	throw UnrecoverableException("ActorInfo::getActorIdForCountry", "Function not defined for derived ActorInfo class");
	return ActorId();
}

// Not implemented in ICEWS

bool ActorInfo::isAGPE(ActorId target) {
	throw UnrecoverableException("ActorInfo::isAGPE", "Function not defined for derived ActorInfo class");
	return false;
}

bool ActorInfo::isAPerson(ActorId target) {
	throw UnrecoverableException("ActorInfo::isAPerson", "Function not defined for derived ActorInfo class");
	return false;
}

bool ActorInfo::isAnOrganization(ActorId target) {
	throw UnrecoverableException("ActorInfo::isAnOrganization", "Function not defined for derived ActorInfo class");
	return false;
}

bool ActorInfo::isAFacility(ActorId target) {
	throw UnrecoverableException("ActorInfo::isAFacility", "Function not defined for derived ActorInfo class");
	return false;
}

Symbol ActorInfo::getEntityTypeForActor(ActorId actor_id) {
	throw UnrecoverableException("ActorInfo::getEntityTypeForActor", "Function not defined for derived ActorInfo class");
	return Symbol();
}

ActorId ActorInfo::getActorIdForGeonameId(std::wstring &geonameid) {
	return ActorId();
}

double ActorInfo::getImportanceScoreForActor(ActorId actor_id) {
	return 0.0;
}

ActorId ActorInfo::getActorByName(std::wstring name) {
	throw UnrecoverableException("ActorInfo::getActorByName", "Function not defined for derived ActorInfo class");
	return ActorId();
}

AgentId ActorInfo::getAgentByName(Symbol name) {
	throw UnrecoverableException("ActorInfo::getAgentByName", "Function not defined for derived ActorInfo class");
	return AgentId();
}

bool ActorInfo::isCountryActorName(std::wstring name) {
	throw UnrecoverableException("ActorInfo::isCountryActorName", "Function not defined for derived ActorInfo class");
	return false;	
}

std::vector<ActorPattern *>& ActorInfo::getPatterns() {
	throw UnrecoverableException("ActorInfo::getPatterns", "Function not defined for derived ActorInfo class");
	return _patterns;
}
