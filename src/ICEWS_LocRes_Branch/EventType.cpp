// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/EventType.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/ICEWSDB.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/Sexp.h"
#include "Generic/common/ParamReader.h"
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <sstream>
#include <boost/scoped_ptr.hpp>

namespace {
	Symbol EVENT_TYPE_SYM(L"event-type");
	Symbol NAME_SYM(L"name");
	Symbol CODE_SYM(L"code");
	Symbol EVENT_GROUP_SYM(L"event-group");
	Symbol DESCRIPTION_SYM(L"description");
	Symbol ROLE_SYM(L"role");
	Symbol ID_SYM(L"id");
	Symbol OVERRIDES_SYM(L"overrides");
	Symbol SOURCE_SYM(L"SOURCE");
	Symbol TARGET_SYM(L"TARGET");
	Symbol DISCARD_EVENTS_SYM(L"DISCARD-EVENTS-WITH-THIS-TYPE");

	// Helper functions for parsing:
	void throwSexpError(const Sexp* sexp, const char* reason) {
		std::stringstream error;
		error << "Parse error while reading ICEWS event-type: " << reason << ": " << sexp->to_debug_string();
		throw UnexpectedInputException("Pattern::throwError()", error.str().c_str());
	}
	void ensureIsListStartingWithAtom(const Sexp* sexp) {
		if ((!sexp->isList()) || (sexp->getNumChildren()==0) || 
			(!sexp->getFirstChild()->isAtom()))
			throwSexpError(sexp, "Expected a list whose first child is an atom");
	}
	std::wstring stripQuotes(Symbol sym) {
		std::wstring s(sym.to_string());
		if ((s.length()>0) && (s[0] == L'"') && (s[s.length()-1]==L'"'))
			return s.substr(1, s.length()-2);
		else
			return s;
	}

}

namespace ICEWS {

bool ICEWSEventType::_event_types_loaded = false;

void ICEWSEventType::readEventTypesFromDatabase() {
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	const char* query = "SELECT eventtype_ID, name, code, description FROM eventtypes";
	size_t num_event_types = 0;
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		ICEWSEventTypeId id(row.getCellAsInt32(0));
		std::wstring name(row.getCellAsWString(1));
		Symbol code(row.getCellAsSymbol(2));
		std::wstring descr(row.getCellAsWString(3));
		std::vector<Symbol> roles;
		roles.push_back(L"SOURCE");
		roles.push_back(L"TARGET");
		registerEventType(boost::make_shared<ICEWSEventType>(id, code, name, descr, roles));
		++num_event_types;
	}
	SessionLogger::info("ICEWS") << "Loaded " << num_event_types << " event types"
		<< "from 'eventtypes' database table";
}

ICEWSEventType::ICEWSEventType(const Sexp* sexp): _discardEvents(false) {
	bool explicit_null_id = false;
	ensureIsListStartingWithAtom(sexp);
	if (sexp->getFirstChild()->getValue() != EVENT_TYPE_SYM)
		throwSexpError(sexp, "Bad event type sexp: expected \"event_type\"");
	for (Sexp* child=sexp->getSecondChild(); child; child=child->getNext()) {
		// Handle atom children.
		if (child->isAtom()) {
			if (child->getValue() == DISCARD_EVENTS_SYM)
				_discardEvents = true;
			else
				throwSexpError(child, "Unexpected child");
			continue;
		}
		// Handle list children.
		ensureIsListStartingWithAtom(child);
		Symbol sym = child->getFirstChild()->getValue();
		if (sym == NAME_SYM) {
			if (!_name.empty())
				throwSexpError(child, "Multiple names specified");
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Expected a single (optionally quoted) atom for name");
			_name = stripQuotes(child->getSecondChild()->getValue().to_string());
		} else if (sym == CODE_SYM) {
			if (!_code.is_null())
				throwSexpError(child, "Multiple codes specified");
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Expected a single (optionally quoted) atom for code");
			_code = stripQuotes(child->getSecondChild()->getValue());
		} else if (sym == EVENT_GROUP_SYM) {
			if (!_eventGroup.is_null())
				throwSexpError(child, "Multiple event groups specified");
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Expected a single (optionally quoted) atom for event group");
			_eventGroup = stripQuotes(child->getSecondChild()->getValue());
		} else if (sym == ID_SYM) {
			if ((_id != ICEWSEventTypeId()) || explicit_null_id)
				throwSexpError(child, "Multiple ids specified");
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Expected a single integer value for id");
			Symbol idValue = child->getSecondChild()->getValue();
			if (boost::iequals(idValue.to_string(), L"NULL"))
				explicit_null_id = true;
			else
				_id = ICEWSEventTypeId(boost::lexical_cast<ICEWSEventTypeId::id_type>(idValue.to_string()));
		} else if (sym == DESCRIPTION_SYM) {
			if (!_description.empty())
				throwSexpError(child, "Multiple descriptions specified");
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Expected a single (optionally quoted) atom for description");
			_description = stripQuotes(child->getSecondChild()->getValue().to_string());
		} else if (sym == ROLE_SYM) {
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Bad role field");
			_roles.push_back(stripQuotes(child->getSecondChild()->getValue()));
		} else if (sym == OVERRIDES_SYM) {
			if ((child->getNumChildren() != 2) || (!child->getSecondChild()->isAtom()))
				throwSexpError(child, "Bad role field");
			std::vector<std::wstring> codes;
			std::wstring arg = stripQuotes(child->getSecondChild()->getValue());
			if (!arg.empty()) {
				boost::split(codes, arg, boost::is_any_of(L",; "));
				// Build a regex that will match the overridden event codes:
				std::wstring overrideRegex;
				BOOST_FOREACH(const std::wstring& code, codes) {
					static const boost::wregex escape(L"([\\.\\[\\{\\}\\(\\)\\+\\?\\|\\^\\$\\\\])");
					static const std::wstring rep(L"\\\\\\1");
					std::wstring codeRegex = boost::regex_replace(code, escape, rep);
					boost::algorithm::replace_all(codeRegex, L"*", L".*");
					if (!overrideRegex.empty())
						overrideRegex += L"|";
					overrideRegex += codeRegex;
				}
				_overrides = boost::make_shared<boost::wregex>(overrideRegex);
			}
		} else {
			throwSexpError(child, "Unexpected child in event type sexp");
		}
	}
	// note: description is optional.
	if (_name.empty())
		throwSexpError(sexp, "No event name specified");
	if (_code.is_null())
		throwSexpError(sexp, "No event code specified");
	if (_eventGroup.is_null())
		throwSexpError(sexp, "No event group specified");
	// Default roles (if none are specified: target & source)
	if (_roles.empty()) {
		_roles.push_back(SOURCE_SYM);
		_roles.push_back(TARGET_SYM);
	}
	// Look up id if it was not specified.
	if (_id.isNull() && !explicit_null_id && !_discardEvents) {
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		std::ostringstream query;
		query << "SELECT eventtype_ID FROM eventtypes WHERE code="
		      << DatabaseConnection::quote(_code.to_string());
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row)
			_id = ICEWSEventTypeId(row.getCellAsInt32(0));
		if (_id.isNull()) {
			throwSexpError(sexp, "Event code not found in database");
		}
	}
}

std::wstring ICEWSEventType::toSexpString() const {
	std::wostringstream out;
	out << L"(event-type\n"
		<< L"  (name \"" << _name << L"\")\n"
		<< L"  (code \"" << _code << L"\")\n";
	if (!_description.empty())
		out << L"  (description \"" << _description << L"\")\n";
	BOOST_FOREACH(Symbol role, _roles)
		out << L"  (role \"" << role.to_string() << L"\")\n";
	out << L")";
	return out.str();
}


ICEWSEventType::ICEWSEventType(ICEWSEventTypeId id, Symbol code, std::wstring name, 
		std::wstring description, const std::vector<Symbol> &roles)
		:_id(id), _code(code), _name(name), _description(description), _roles(roles), _discardEvents(false)
{}

Symbol ICEWSEventType::getEventCode() const {
	return _code;
}

bool ICEWSEventType::hasRole(Symbol role) const {
	return (std::find(_roles.begin(), _roles.end(), role) != _roles.end());
}

std::vector<Symbol> ICEWSEventType::getRoles() const {
	return _roles;
}

std::vector<Symbol> ICEWSEventType::getRequiredRoles() const {
	return _roles; // For now, all roles are required.
}

std::wstring ICEWSEventType::getName() const {
	return _name;
}

ICEWSEventTypeId ICEWSEventType::getEventId() const {
	return _id;
}

Symbol ICEWSEventType::getEventGroup() const {
	return _eventGroup;
}

bool ICEWSEventType::discardEventsWithThisType() const {
	return _discardEvents;
}


Symbol::HashMap<ICEWSEventType_ptr> &ICEWSEventType::_typeRegistry() {
	static Symbol::HashMap<ICEWSEventType_ptr> registry;
	return registry;
}

ICEWSEventType_ptr ICEWSEventType::getEventTypeForCode(Symbol code) {
	if (!_event_types_loaded)
		ICEWSEventType::loadEventTypes();
	return _typeRegistry()[code];
}

void ICEWSEventType::registerEventType(ICEWSEventType_ptr eventType) {
	ICEWSEventType_ptr& dst = _typeRegistry()[eventType->_code];
	if (dst) {
		// Should we check if they match, and if so then just ignore it?
		std::wstringstream err;
		err << L"Event type \"" << eventType->_code.to_string() << L"\" already registered.";
		throw InternalInconsistencyException("ICEWSEventType::registerEventTypeForCode", err);
	}
	dst = eventType; // note: dst is a reference.
}

void ICEWSEventType::loadEventTypes() {
	if (_event_types_loaded) return;
	/*
	if (ParamReader::hasParam("icews_event_types") && ParamReader::hasParam("icews_db"))
		throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder",
			"You should define EITHER the 'icews_event_types' parameter OR the 'icews_db' parameter, but not both");
	*/
	if (ParamReader::hasParam("icews_event_types")) {
		ICEWS::ICEWSEventType::readEventTypesFromFile();
	} else if (ParamReader::hasParam("icews_db")) {
		ICEWS::ICEWSEventType::readEventTypesFromDatabase();
	} else {
		throw UnexpectedInputException("ICEWSEventMentionFinder::ICEWSEventMentionFinder",
			"You must define either the 'icews_event_types' parameter or the "
			" icews_db parameter, to specify the ICEWS event types.");
	}
	_event_types_loaded = true;
}

void ICEWSEventType::readEventTypesFromFile() {
	std::string filename = ParamReader::getParam("icews_event_types");
	if (!filename.empty()) {
		boost::scoped_ptr<UTF8InputStream> istream_scoped_ptr(UTF8InputStream::build(filename.c_str()));
		UTF8InputStream& istream(*istream_scoped_ptr);
		while (!istream.eof()) {
			Sexp eventTypeSexp(istream, true, true);
			if (eventTypeSexp.isVoid()) break;
			ICEWSEventType_ptr eventType(_new ICEWSEventType(&eventTypeSexp));
			registerEventType(eventType);
		}
	}
}

bool ICEWSEventType::overridesEventType(ICEWSEventType_ptr other) const {
	if (_overrides)
		return boost::regex_match(other->_code.to_string(), *_overrides);
	else
		return false;
}

bool ICEWSEventType::rolesCanBeFilledByTheSameEntity(Symbol role1, Symbol role2) const {
	return false;
}



} // end of namespace ICEWS
