// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_INFO_H
#define ICEWS_ACTOR_INFO_H

#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorInfo.h"
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
class ICEWSActorInfo : public ActorInfo {
public:
	ICEWSActorInfo();
	~ICEWSActorInfo();

	static boost::shared_ptr<ActorInfo> getICEWSActorInfo();

	/** Return the name for the given actor (as specified by the
	* table dict_actors.name) */
	virtual std::wstring getActorName(ActorId target);

	/** Return the name for the given agent (as specified by the
	* table dict_agents.name) */
	virtual std::wstring getAgentName(AgentId target);

	/** Return the unique_code for the given actor (as specified
	* by the table dict_actors.unique_code) */
	virtual Symbol getActorCode(ActorId target);

	/** Return the agent_code for the given actor (as specified
	* by the table dict_agents.agent_code) */
	virtual Symbol getAgentCode(AgentId target);

	/** Return the name for the given sector */
	virtual std::wstring getSectorName(Symbol sectorCode);
	virtual std::wstring getSectorName(SectorId target);

	/** Return the ActorID for an actor, given its agent_code. */
	virtual ActorId getActorByCode(Symbol actorCode);

	/** Return the ActorID for a country, given its country_id. */
	virtual ActorId getActorIdForCountry(CountryId countryId);

	/** Return the CountryID for the given actor if it is a country,
	* or the NULL CountryID otherwise. */
	virtual CountryId getCountryId(ActorId target);

	/** Return true if the given actor corresponds to a country. */
	virtual bool isACountry(ActorId target);

	/** Return true if the given actor corresponds to an individual. */
	virtual bool isAnIndividual(ActorId target);

	/** Return true if the given actor is a country, or if it looks 
	* like it might be a city or some other location within a 
	* country. */
	virtual bool mightBeALocation(ActorId target);

	/** Return true if the given agent may only be paired with 
	* country actors. */
	virtual bool isRestrictedToCountryActors(AgentId target);

	/** Return a list of actors that dominate the given actor in
	* the ICEWS actor hierarchy.  If a date is specified, then
	* only return actors that dominated the given target actor
	* on the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedActorIds(ActorId target, const char *date=0);

	/** Not usd in ICEWS; just return country actor IDs instead */
	virtual std::vector<ActorId> getAssociatedLocationActorIds(ActorId target, const char *date=0);

	/** Return a list of countries that dominate the given actor in
	* the ICEWS actor hierarchy.   If a date is specified, then
	* only return countries that dominated the given target actor
	* on the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<CountryId> getAssociatedCountryIds(ActorId target, const char *date=0);

	/** Return a list of actor ids for countries that dominate the 
	* given actor in the ICEWS actor hierarchy.   If a date is 
	* specified, then only return countries that dominated the 
	* given target actor on the specified date.  Date should be 
	* YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedCountryActorIds(ActorId target, const char *date=0);

	/** Return a list of the ids of sectors that the given actor belongs
	* to (based on the ICEWS sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<SectorId> getAssociatedSectorIds(ActorId target, const char *date=0);

	/** Return a list of the codes of sectors that the given actor belongs
	* to (based on the ICEWS sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<Symbol> getAssociatedSectorCodes(ActorId target, const char *date=0, int min_frequency=-1024);

	/** Return a list of the codes of sectors that the given agent belongs
	* to (based on the ICEWS sector mappings table). */
	virtual std::vector<Symbol> getAssociatedSectorCodes(AgentId target);

	/** Return the country ISO Code for a particular actor ID */
	virtual Symbol getIsoCodeForActor(ActorId actor_id);
	
	/** Used only in ICEWS to assign persons to a default agent category */
	virtual AgentId getDefaultPersonAgentId();
	virtual Symbol getDefaultPersonAgentCode();

	void preCacheActorInfo(const ActorId::HashSet& actorIds);
private:
	struct DictActorRow;
	struct DictAgentRow;
	struct Cache;
	Cache *_cache;
	
	static bool _initialized;
	static boost::shared_ptr<ICEWSActorInfo> _actorInfo;

};
typedef boost::shared_ptr<ICEWSActorInfo> ICEWSActorInfo_ptr;


#endif
