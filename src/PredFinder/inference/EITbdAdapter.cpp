#include "Generic/common/leak_detection.h"
#include "Generic/common/SessionLogger.h"
#include <string>
#include "PredFinder/common/ContainerTypes.h"
#include "EITbdAdapter.h"

const PairOfStrings AGENT_ROLE_MAPPINGS[] = {
	PairOfStrings(L"bombing", L"eru:mediatingAgent"),
	PairOfStrings(L"attack", L"eru:mediatingAgent"),
	PairOfStrings(L"killing", L"eru:killingHumanAgent"),
	PairOfStrings(L"injury", L"eru:injuringHumanAgent")
};

const size_t AGENT_ROLE_MAPPINGS_COUNT = sizeof(AGENT_ROLE_MAPPINGS)/sizeof(PairOfStrings);

const PairOfStrings EVENT_ROLE_MAPPINGS[] = {
	// "eru:genericViolence" is not an actual IC class; 
	// used to get dates and locations for real attacks
	PairOfStrings(L"generic_violence", L"eru:genericViolence"),
	PairOfStrings(L"bombing", L"eru:bombing"),
	PairOfStrings(L"attack", L"eru:physicalDamageEvent"),
	PairOfStrings(L"killing", L"eru:humanKillingEvent"),
	PairOfStrings(L"injury", L"eru:humanInjuryEvent")
};

const size_t EVENT_ROLE_MAPPINGS_COUNT = sizeof(EVENT_ROLE_MAPPINGS)/sizeof(PairOfStrings);

// NOTE: If you add a new output string to getPatientRole() below, be sure to add it here as well!
const std::wstring PATIENT_ROLES[] = {
	L"eru:eventLocationGPE", 
	L"eru:eventLocation",
	L"eru:thingPhysicallyDamaged",
	L"eru:personGroupKilled",
	L"eru:personKilled",
	L"eru:personGroupInjured",
	L"eru:personInjured"
};

const size_t PATIENT_ROLES_COUNT = sizeof(PATIENT_ROLES)/sizeof(std::wstring);

///// START OF PATIENT ROLE TABLES (NOT USED YET)
const PairOfStrings BOMBING_OR_ATTACK_PAT_ROLES[] = {
	// when tbd_event_type == "bombing" or "attack", use entity type to get patient role
	PairOfStrings(L"GPE", L"eru:eventLocationGPE"),
	PairOfStrings(L"LOC", L"eru:eventLocation"),
	PairOfStrings(L"FAC", L"eru:thingPhysicallyDamaged"),
	PairOfStrings(L"PER", L"eru:thingPhysicallyDamaged"),
	PairOfStrings(L"ORG", L"eru:thingPhysicallyDamaged"),
	PairOfStrings(L"VEH", L"eru:thingPhysicallyDamaged")
	// else invalid
};
const size_t BOMBING_OR_ATTACK_PAT_ROLES_COUNT = sizeof(BOMBING_OR_ATTACK_PAT_ROLES)/sizeof(PairOfStrings);

const PairOfStrings KILLING_PAT_ROLE = 
	//if est==L"Group", then this	// else this
	PairOfStrings(L"eru:personGroupKilled", L"eru:personKilled");

const PairOfStrings INJURY_PAT_ROLE = 
	//if est==L"Group", then this	// else this
	PairOfStrings(L"eru:personGroupInjured", L"eru:personInjured");

///// END OF PATIENT ROLES TABLES (NOT USED YET)

const PairOfStrings EVENT_WITH_AGENT_NAME[] = {
	// These two items intentionally duplicate the first two in EVENT_WITHOUT_AGENT_NAME.
	PairOfStrings(L"attack", L"eru:genericAttackEvent"), 
	PairOfStrings(L"bombing", L"eru:bombingAttackEvent"),

	PairOfStrings(L"killing", L"eru:killingWithAgentEvent"),
	PairOfStrings(L"injury", L"eru:injuryWithAgentEvent")
};

const PairOfStrings EVENT_WITHOUT_AGENT_NAME[] = {
	PairOfStrings(L"attack", L"eru:genericAttackEvent"),
	PairOfStrings(L"bombing", L"eru:bombingAttackEvent"),
	PairOfStrings(L"killing", L"eru:killingWithoutAgentEvent"),
	PairOfStrings(L"injury", L"eru:injuryWithoutAgentEvent")
};

const size_t EVENT_WITH_AGENT_NAME_COUNT = sizeof(EVENT_WITH_AGENT_NAME)/sizeof(PairOfStrings);
const size_t EVENT_WITHOUT_AGENT_NAME_COUNT = sizeof(EVENT_WITHOUT_AGENT_NAME)/sizeof(PairOfStrings);

bool EITbdAdapter::_initialized = false;
StringReplacementMap EITbdAdapter::_agent_role;
StringReplacementMap EITbdAdapter::_event_role;
StringReplacementMap EITbdAdapter::_event_with_agent_name;
StringReplacementMap EITbdAdapter::_event_without_agent_name;

/**
 * Must be called before calling any other EITbdAdapter methods.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.21
 **/
void EITbdAdapter::init() {
	if (_initialized) {
		return;
	}
	for (size_t i = 0; i < AGENT_ROLE_MAPPINGS_COUNT; ++i) {
		_agent_role[AGENT_ROLE_MAPPINGS[i].first] = AGENT_ROLE_MAPPINGS[i].second;
	}
	for (size_t i = 0; i < EVENT_ROLE_MAPPINGS_COUNT; ++i) {
		_event_role[EVENT_ROLE_MAPPINGS[i].first] = EVENT_ROLE_MAPPINGS[i].second;
	}
	for (size_t i = 0; i < EVENT_WITH_AGENT_NAME_COUNT; ++i) {
		_event_with_agent_name[EVENT_WITH_AGENT_NAME[i].first] = 
			EVENT_WITH_AGENT_NAME[i].second;
	}
	for (size_t i = 0; i < EVENT_WITHOUT_AGENT_NAME_COUNT; ++i) {
		_event_without_agent_name[EVENT_WITHOUT_AGENT_NAME[i].first] = 
			EVENT_WITHOUT_AGENT_NAME[i].second;
	}
	_initialized = true;
}

void EITbdAdapter::throw_if_not_initialized() {
	if (!_initialized) {
		std::runtime_error e("Called EITbdAdapter method before calling EITbdAdapter::init()");
		throw e;
	}
}

/**
 * Get appropriate agent role for this event and argument, e.g. eru:mediatingAgent. 
 * The entity subtype does NOT include a "guessed default", so you must account
 * for UNDET.
 * @example getAgentRole(L"bombing") -> L"eru:mediatingAgent"
 * @example getAgentRole(L"attack") -> L"eru:mediatingAgent"
 * @example getAgentRole(L"killing") -> L"eru:killingHumanAgent"
 * @example getAgentRole(L"junk") -> L""
 *
 * @param tbd_agent_type Name of the agent type from the pattern set
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getAgentRole(const std::wstring & tbd_event_type) {
	throw_if_not_initialized();
	StringReplacementMap::const_iterator iter = _agent_role.find(tbd_event_type);
	if (iter == _agent_role.end()) {
		return L"";
	} else {
		return iter->second;
	}
}

/**
 * Get all possible agent roles.
 * @return Set of strings containing all possible output agent roles.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.21
 **/
SetOfStrings EITbdAdapter::getAgentRoles() {
	SetOfStrings ret;
	BOOST_FOREACH(PairOfStrings str_pair, _agent_role) {
		ret.insert(str_pair.second);
	}
	return ret;
}

/**
 * Get appropriate agent type for this event and argument, e.g. ic:HumanAgent. 
 * The entity subtype does NOT include a "guessed default", so you must account
 * for UNDET.
 *
 * @param et Entity type of the argument
 * @param est Entity subtype of the argument
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getAgentType(const EntityType & et, const EntitySubtype & est) {
	throw_if_not_initialized();
	// Currently the same for all types of events
	if (et.matchesPER()) {
		if (est.getName() == Symbol(L"Group")) {
			return L"ic:PersonGroup";
		} else {
			return L"ic:Person";
		}
	} else if (et.matchesGPE() || et.matchesORG()) {
		return L"ic:HumanAgent";
	} 
	return L"";
}

/**
 * Get appropriate patient role for this event and argument, e.g. eru:eventLocation. 
 * The entity subtype does NOT include a "guessed default", so you must account
 * for UNDET.
 *
 * @param tbd_event_type Name of the event type from the pattern set
 * @param et Entity type of the argument
 * @param est Entity subtype of the argument
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getPatientRole(const std::wstring & tbd_event_type, 
												  const EntityType & et, const EntitySubtype & est) {
	throw_if_not_initialized();
	// NOTE: If you add a new output string to this method, be sure to add it to PATIENT_ROLES above 
	// as well!
	if (tbd_event_type.compare(L"bombing") == 0 || tbd_event_type.compare(L"attack") == 0) {
		if (et.matchesGPE())
			return L"eru:eventLocationGPE";
		else if (et.matchesLOC())
			return L"eru:eventLocation";
		else if (et.matchesFAC() || et.matchesPER() || et.matchesORG())
			return L"eru:thingPhysicallyDamaged"; // not sure about ORG/FAC
		else if (et.getName() == Symbol(L"VEH"))
			return L"eru:thingPhysicallyDamaged";
		else SessionLogger::info("LEARNIT") << "invalid target for " << tbd_event_type << "\n";
	} else if (tbd_event_type.compare(L"killing") == 0) {
		if (est.getName() == Symbol(L"Group"))
			return L"eru:personGroupKilled";
		else return L"eru:personKilled";			
	} else if (tbd_event_type.compare(L"injury") == 0) {
		if (est.getName() == Symbol(L"Group"))
			return L"eru:personGroupInjured";
		else return L"eru:personInjured";			
	}
	
	return L"";
}

/**
 * Get all possible patient roles.
 * @return Set of strings containing all possible output patient roles.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.21
 **/
SetOfStrings EITbdAdapter::getPatientRoles() {
	return std::set<std::wstring>(PATIENT_ROLES, PATIENT_ROLES + PATIENT_ROLES_COUNT);
}

/**
 * Get appropriate patient type for this event and argument, e.g. ic:Person. 
 * The entity subtype does NOT include a "guessed default", so you must account
 * for UNDET.
 *
 * @param tbd_event_type Name of the event type from the pattern set
 * @param et Entity type of the argument
 * @param est Entity subtype of the argument
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getPatientType(const std::wstring & tbd_event_type, 
												  const EntityType & et, const EntitySubtype & est) {
	throw_if_not_initialized();
	if (tbd_event_type.compare(L"bombing") == 0 || tbd_event_type.compare(L"attack") == 0) {
		if (et.matchesGPE())
			return L"ic:GeopoliticalEntity";
		else if (et.matchesLOC())
			return L"ic:Location";
		else if (et.matchesFAC() || et.matchesPER() || et.matchesORG())
			return L"ic:PhysicalThing"; 
		else if (et.getName() == Symbol(L"VEH"))
			return L"ic:PhysicalThing";
		else SessionLogger::info("LEARNIT") << "invalid target for " << tbd_event_type << "\n";
	} else if (tbd_event_type.compare(L"killing") == 0 || tbd_event_type.compare(L"injury") == 0) {
		if (est.getName() == Symbol(L"Group"))
			return L"ic:PersonGroup";
		else return L"ic:Person";
	} 
	
	return L"";
}

/**
 * Get appropriate ontology role for this event.
 * @example "bombing" -> "eru:bombing"
 * @example "attack" -> "eru:physicalDamageEvent"
 * @example "junk" -> ""
 *
 * @param tbd_event_type Name of the event type from the pattern set
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getEventRole(const std::wstring & tbd_event_type) {	
	throw_if_not_initialized();
	StringReplacementMap::const_iterator iter = _event_role.find(tbd_event_type);
	if (iter == _event_role.end()) {
		return L"";
	} else {
		return iter->second;
	}
}

/**
 * Get all possible event roles.
 * @return Set of strings containing all possible output event roles.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.21
 **/
SetOfStrings EITbdAdapter::getEventRoles() {
	SetOfStrings ret;
	BOOST_FOREACH(PairOfStrings str_pair, _event_role) {
		ret.insert(str_pair.second);
	}
	return ret;
}

/**
 * Get appropriate ontology type for this event e.g. ic:Bombing. 
 *
 * @param tbd_event_type Name of the event type from the pattern set
 * @param has_agent Whether this event has an agent
 *
 * @author eboschee@bbn.com
 * @date 2010.12.12
 **/
std::wstring EITbdAdapter::getEventType(const std::wstring & tbd_event_type, bool has_agent) {
	throw_if_not_initialized();
	if (tbd_event_type.compare(L"generic_violence") == 0)
		return L"NONE"; // not an actual IC class; used to get dates and locations for real attacks

	if (tbd_event_type.compare(L"bombing") == 0)
		return L"ic:Bombing";

	if (tbd_event_type.compare(L"attack") == 0)
		return L"ic:HumanAgentPhysicallyAttacking";
			
	if (tbd_event_type.compare(L"killing") == 0) {
		if (has_agent)
			return L"ic:HumanAgentKillingAPerson";
		else return L"ic:HumanKillingEvent";
	}

	if (tbd_event_type.compare(L"injury") == 0) {
		if (has_agent)
			return L"ic:HumanAgentInjuringAPerson";
		else return L"ic:HumanInjuryEvent";
	}

	return L"";
}

/**
 * Get appropriate internal name for this event e.g. eru:bombingAttackEvent
 *
 * @param tbd_event_type Name of the event type from the pattern set
 * @param has_agent Whether this event has an agent
 *
 * @author eboschee@bbn.com
 * @date 2010.12.14
 **/
std::wstring EITbdAdapter::getEventName(const std::wstring & tbd_event_type, bool has_agent) {
	throw_if_not_initialized();
	if (has_agent) {
		StringReplacementMap::const_iterator iter = _event_with_agent_name.find(tbd_event_type);
		if (iter == _event_with_agent_name.end()) {
			return L"";
		} else {
			return iter->second;
		}
	} else {
		StringReplacementMap::const_iterator iter = _event_without_agent_name.find(tbd_event_type);
		if (iter == _event_without_agent_name.end()) {
			return L"";
		} else {
			return iter->second;
		}
	}
}

/**
 * Get all possible event names.
 * @return Set of strings containing all possible output event names.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.21
 **/
SetOfStrings EITbdAdapter::getEventNames() {
	throw_if_not_initialized();
	SetOfStrings ret;
	BOOST_FOREACH(PairOfStrings str_pair, _event_with_agent_name) {
		ret.insert(str_pair.second);
	}
	BOOST_FOREACH(PairOfStrings str_pair, _event_without_agent_name) {
		ret.insert(str_pair.second);
	}
	return ret;
}
