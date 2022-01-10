// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_EVENT_TYPE_H
#define ICEWS_EVENT_TYPE_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "Generic/common/Symbol.h"
#include "Generic/common/BoostUtil.h"
#include "Generic/actors/Identifiers.h"
#include <vector>
#include <boost/regex_fwd.hpp>

class Sexp;


/** A record used to store information about a single type of ICEWES
  * event.  Each ICEWSEventType has a unique CAMEO event code and a
  * set of roles.  In addition, it records information about those
  * roles (such as which roles are required, and how they relate to
  * one another).
  *
  * ICEWSEventTypes are immutable and non-copayble.  
  *
  * Static methods are used to define a global registry of event types.
  * This registry is typically initialized by calling the static
  * method ICEWSEventType::loadEventTypes().
  *
  * This class may be expanded in the future to allow for additional
  * customization of event types.
  */
class ICEWSEventType : private boost::noncopyable {
public:	
	BOOST_MAKE_SHARED_1ARG_CONSTRUCTOR(ICEWSEventType, const Sexp*);
	BOOST_MAKE_SHARED_5ARG_CONSTRUCTOR(ICEWSEventType, ICEWSEventTypeId, Symbol, std::wstring, std::wstring, const std::vector<Symbol>&);

	/** Return the CAMEO event code specifying what kind of event is
	  * identified by this event mention.  For a list of CAMEO event
	  * codes, see <http://cameocodes.wikispaces.com/EventCodes> */
	Symbol getEventCode() const;

	/** Return the primary key in eventtypes table for this event. */
	ICEWSEventTypeId getEventId() const;

	/** Return true if this event type has a role with the given name. */
	bool hasRole(Symbol role) const;

	/** Return a list of the roles that events with this event type can take. */
	std::vector<Symbol> getRoles() const;

	/** Return a list of the roles that events with this event type must take. */
	std::vector<Symbol> getRequiredRoles() const;

	/** Return an s-expression string describing this event type. */
	std::wstring toSexpString() const;

	std::wstring getName() const;

	Symbol getEventGroup() const;

	bool discardEventsWithThisType() const;

	/** Return true if this event type should "override" the given event type;
	  * i.e., if we find two events with the same participants, one with this
	  * event type, and one with the given event type, then return true if 
	  * the event with this event type should take precedence. */
	bool overridesEventType(boost::shared_ptr<ICEWSEventType> other) const;

	/*=========================== Registry ============================ */
	static boost::shared_ptr<ICEWSEventType> getEventTypeForCode(Symbol code);

	/** Register a set of event types */
	static void loadEventTypes();

	/** Manually register an event type. */
	static void registerEventType(boost::shared_ptr<ICEWSEventType> eventType);


private:
	ICEWSEventType(const Sexp* sexp);
	ICEWSEventType(ICEWSEventTypeId id, Symbol code, std::wstring name, 
		std::wstring description, const std::vector<Symbol> &roles);
	static void readEventTypesFromDatabase();
	static void readEventTypesFromFile();
	static bool _event_types_loaded;

	ICEWSEventTypeId _id;        /** Primary key in eventtypes table */
	Symbol _code;                /** CAMEO code */
	std::wstring _name;          /** Short (1-5 word) name for event type */
	std::wstring _description;   /** Description of event type */
	std::vector<Symbol> _roles;  /** List of roles that this event type takes */
	std::vector<Symbol> _requiredRoles;  /** List of REQUIRED roles that this event type takes */
	boost::shared_ptr<boost::wregex> _overrides; /** List of CAMEO codes that this event type overrides */
	Symbol _eventGroup;          /** Name of a group that this event belongs to */
	bool _discardEvents;         /** If true, then events with this type should be discarded in the final output */

	static Symbol::HashMap<boost::shared_ptr<ICEWSEventType> > &_typeRegistry();
};
typedef boost::shared_ptr<ICEWSEventType> ICEWSEventType_ptr;

#endif
