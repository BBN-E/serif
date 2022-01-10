// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_INFO_H
#define ACTOR_INFO_H

#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorPattern.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

// Forward declaration
class DatabaseConnection;
typedef boost::shared_ptr<DatabaseConnection> DatabaseConnection_ptr;

/** Class used to look up information about actors.  This information 
* could either be looked up from database tables or from some other
* source (such as files). 
*/
class ActorInfo : private boost::noncopyable {
public:
	ActorInfo();
	~ActorInfo();

	/** Returns the appropriate ActorInfo, depending on the parameter */
	static boost::shared_ptr<ActorInfo> getAppropriateActorInfoForICEWS();

	/** Return the name for the given actor (as specified by the
	* table dict_actors.name) */
	virtual std::wstring getActorName(ActorId target) = 0;

	/** Return the name for the given agent (as specified by the
	* table dict_agents.name) */
	virtual std::wstring getAgentName(AgentId target) = 0;

	/** Return the unique_code for the given actor (as specified
	* by the table dict_actors.unique_code) */
	virtual Symbol getActorCode(ActorId target) = 0;

	/** Return the agent_code for the given actor (as specified
	* by the table dict_agents.agent_code) */
	virtual Symbol getAgentCode(AgentId target) = 0;

	/** Return the name for the given sector */
	virtual std::wstring getSectorName(Symbol sectorCode); // ICEWS only
	virtual std::wstring getSectorName(SectorId target) = 0;

	/** Overloaded method for getting name */
	virtual std::wstring getName(ActorId target) { return getActorName(target); }
	virtual std::wstring getName(AgentId target) { return getAgentName(target); }
	virtual std::wstring getName(SectorId target) { return getSectorName(target); }
	virtual std::wstring getName(CountryId target) { return getActorName(getActorIdForCountry(target)); }

	/** Return the ActorID for an actor, given its actor_code. */
	virtual ActorId getActorByCode(Symbol actorCode); // ICEWS only

	/** Return the ActorID for a country, given its country_id. */
	virtual ActorId getActorIdForCountry(CountryId countryId); // ICEWS only

	/** Return the CountryID for the given actor if it is a country,
	* or the NULL CountryID otherwise. */
	virtual CountryId getCountryId(ActorId target) = 0;

	/** Return true if the given actor corresponds to a country. */
	virtual bool isACountry(ActorId target) = 0;

	/** Return true if the given actor corresponds to an individual. */
	virtual bool isAnIndividual(ActorId target) = 0;

	/** Return the entity type for a particular actor ID if explicit in the actor DB */
	virtual Symbol getEntityTypeForActor(ActorId actor_id);

	/** Return true if the given actor is a GPE */
	virtual bool isAGPE(ActorId target);
	
	/** Return true if the given actor is a Person */
	virtual bool isAPerson(ActorId target);

	/** Return true if the given actor is an Organization */
	virtual bool isAnOrganization(ActorId target);

	/** Return true if the given actor is an Organization */
	virtual bool isAFacility(ActorId target);

	/** Return true if the given actor is a country, or if it looks 
	* like it might be a city or some other location within a 
	* country. */
	virtual bool mightBeALocation(ActorId target) = 0;

	/** Return true if the given agent may only be paired with 
	* country actors. */
	virtual bool isRestrictedToCountryActors(AgentId target) = 0;

	/** Return a list of actors that dominate the given actor in
	* the ICEWS actor hierarchy.  If a date is specified, then
	* only return actors that dominated the given target actor
	* on the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedActorIds(ActorId target, const char *date=0) = 0;
	
	/** Returns a list of actor ids that are linked to the given
	  * actor as a Location. */
	virtual std::vector<ActorId> getAssociatedLocationActorIds(ActorId target, const char *date=0) = 0;

	/** Return a list of countries that dominate the given actor in
	* the ICEWS actor hierarchy.   If a date is specified, then
	* only return countries that dominated the given target actor
	* on the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<CountryId> getAssociatedCountryIds(ActorId target, const char *date=0) = 0;

	/** Return a list of actor ids for countries that dominate the 
	* given actor in the ICEWS actor hierarchy.   If a date is 
	* specified, then only return countries that dominated the 
	* given target actor on the specified date.  Date should be 
	* YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedCountryActorIds(ActorId target, const char *date=0) = 0;

	/** Return a list of the ids of sectors that the given actor belongs
	* to (based on the ICEWS sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<SectorId> getAssociatedSectorIds(ActorId target, const char *date=0) = 0;

	/** Return a list of the codes of sectors that the given actor belongs
	* to (based on the ICEWS sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<Symbol> getAssociatedSectorCodes(ActorId target, const char *date=0, int min_frequency=-1024) = 0;

	/** Return a list of the codes of sectors that the given agent belongs
	* to (based on the ICEWS sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<Symbol> getAssociatedSectorCodes(AgentId target) = 0;

	/** Return the country ISO Code for a particular actor ID */
	virtual Symbol getIsoCodeForActor(ActorId actor_id) = 0;

	/** If the database contains actors imported from geonames, that is 
	    actors with a geonameid field, then return the ActorId for a 
		particular genonameid. Otherwise return null ActorId */
	virtual ActorId getActorIdForGeonameId(std::wstring &geonameid);

	/** ImportanceScore of actor */
	virtual double getImportanceScoreForActor(ActorId target);

	/** Return the ActorID for an actor, given its canonical name. */
	virtual ActorId getActorByName(std::wstring name);

	/** Return the AgentId for an agent, given its canonical name. */
	virtual AgentId getAgentByName(Symbol name);

	/** Used only in ICEWS to assign persons to a default agent category */
	virtual AgentId getDefaultPersonAgentId() = 0;
	virtual Symbol getDefaultPersonAgentCode() = 0;

	/** Case insensitive check to see if name is country actor name */
	virtual bool isCountryActorName(std::wstring name);

	/** List of patterns loaded from actor database (used only in AWAKEActorInfo **/
	virtual std::vector<ActorPattern *>& getPatterns();

protected:

	std::vector<ActorPattern *> _patterns;
};
typedef boost::shared_ptr<ActorInfo> ActorInfo_ptr;


#endif
