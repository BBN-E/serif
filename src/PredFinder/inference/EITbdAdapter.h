#include "boost/foreach.hpp"
#include "boost/make_shared.hpp"
#include "Generic/common/Offset.h"
#include "Generic/theories/EntityType.h"
#include "Generic/theories/EntitySubtype.h"
#include "boost/tuple/tuple.hpp"
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include "PredFinder/common/ContainerTypes.h"

#pragma once

class EITbdAdapter {
public:
	// must be called before any other methods are used, or an exception will be thrown
	static void init(); 
	// Canonical role/type values for tbd events and their arguments
	// Uses simple strings ("bombing", "killing", "injury", "attack") to represent tbd event types
	// and maps them to, e.g., "eru:mediatingAgent". "Agent" is the actor, while "patient" is 
	// what is acted upon.
	static std::wstring getAgentRole(const std::wstring &tbd_event_type);
	static std::wstring getAgentType(const EntityType & et, const EntitySubtype & est);
	static std::wstring getPatientRole(const std::wstring &tbd_event_type, 
		const EntityType & et, const EntitySubtype & est);
	static std::wstring getPatientType(const std::wstring &tbd_event_type, 
		const EntityType & et, const EntitySubtype & est);
	static std::wstring getEventRole(const std::wstring &tbd_event_type);
	static std::wstring getEventType(const std::wstring &tbd_event_type, bool has_agent = false);	
	static std::wstring getEventName(const std::wstring &tbd_event_type, bool has_agent = false);	

	static SetOfStrings getAgentRoles();
	static SetOfStrings getPatientRoles();
	static SetOfStrings getEventRoles();
	static SetOfStrings getEventNames();

private:
	// Data members
	static bool _initialized;
	static void throw_if_not_initialized();
	static StringReplacementMap _agent_role;
	static StringReplacementMap _event_role;
	static StringReplacementMap _event_with_agent_name;
	static StringReplacementMap _event_without_agent_name;

	// Constructor (intentionally made private)
	EITbdAdapter() {}
	~EITbdAdapter() {}
};
