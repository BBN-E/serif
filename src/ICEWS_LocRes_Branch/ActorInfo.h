// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_ACTOR_INFO_H
#define ICEWS_ACTOR_INFO_H

#include "ICEWS/Identifiers.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

// Forward declaration
class DatabaseConnection;
typedef boost::shared_ptr<DatabaseConnection> DatabaseConnection_ptr;

namespace ICEWS {

	/** Class used to look up information about actors.  This information 
	  * could either be looked up from database tables or from some other
	  * source (such as files). 
	  */
	class ActorInfo: private boost::noncopyable {
	public:
		ActorInfo();
		~ActorInfo();

		/** Return a pointer to a singleton ActorInfo object.  This 
		  * lets us avoid having to create a new ActorInfo object 
		  * each time we want to look something up. */
		static boost::shared_ptr<ActorInfo> getActorInfoSingleton();

		/** Return the name for the given actor (as specified by the
		  * table dict_actors.name) */
		std::wstring getActorName(ActorId target);

		/** Return the name for the given agent (as specified by the
		  * table dict_agents.name) */
		std::wstring getAgentName(AgentId target);

		/** Return the unique_code for the given actor (as specified
		  * by the table dict_actors.unique_code) */
		Symbol getActorCode(ActorId target);

		/** Return the agent_code for the given actor (as specified
		  * by the table dict_agents.agent_code) */
		Symbol getAgentCode(AgentId target);

		/** Return the name for the given sector */
		std::wstring getSectorName(Symbol sectorCode);
		std::wstring getSectorName(SectorId target);

		/** Overloaded method for getting name */
		std::wstring getName(ActorId target) { return getActorName(target); }
		std::wstring getName(AgentId target) { return getAgentName(target); }
		std::wstring getName(SectorId target) { return getSectorName(target); }
		std::wstring getName(CountryId target) { return getActorName(getActorIdForCountry(target)); }

		/** Return the AgentID for an agent, given its agent_code. */
		AgentId getAgentByCode(Symbol agentCode);

		/** Return the ActorID for an actor, given its agent_code. */
		ActorId getActorByCode(Symbol actorCode);

		/** Return the ActorID for a country, given its country_id. */
		ActorId getActorIdForCountry(CountryId countryId);

		/** Return the CountryID for the given actor if it is a country,
		  * or the NULL CountryID otherwise. */
		CountryId getCountryId(ActorId target);

		/** Return true if the given actor corresponds to a country. */
		bool isACountry(ActorId target);

		/** Return true if the given actor corresponds to an individual. */
		bool isAnIndividual(ActorId target);

		/** Return true if the given actor is a country, or if it looks 
		  * like it might be a city or some other location within a 
		  * country. */
		bool mightBeALocation(ActorId target);

		/** Return true if the given agent may only be paired with 
		  * country actors. */
		bool isRestrictedToCountryActors(AgentId target);

		/** Return a list of actors that dominate the given actor in
		  * the ICEWS actor hierarchy.  If a date is specified, then
		  * only return actors that dominated the given target actor
		  * on the specified date.  Date should be YYYY-MM-DD. */
		std::vector<ActorId> getAssociatedActorIds(ActorId target, const char *date=0);

		/** Return a list of countries that dominate the given actor in
		  * the ICEWS actor hierarchy.   If a date is specified, then
		  * only return countries that dominated the given target actor
		  * on the specified date.  Date should be YYYY-MM-DD. */
		std::vector<CountryId> getAssociatedCountryIds(ActorId target, const char *date=0);

		/** Return a list of actor ids for countries that dominate the 
		  * given actor in the ICEWS actor hierarchy.   If a date is 
		  * specified, then only return countries that dominated the 
		  * given target actor on the specified date.  Date should be 
		  * YYYY-MM-DD. */
		std::vector<ActorId> getAssociatedCountryActorIds(ActorId target, const char *date=0);

		/** Return a list of the ids of sectors that the given actor belongs
		  * to (based on the ICEWS sector mappings table).  If a date is
		  * specified, then only return sectors that the given target
		  * actor belong to on that date.  Date should be YYYY-MM-DD. */
		std::vector<SectorId> getAssociatedSectorIds(ActorId target, const char *date=0);

		/** Return a list of the codes of sectors that the given actor belongs
		  * to (based on the ICEWS sector mappings table).  If a date is
		  * specified, then only return sectors that the given target
		  * actor belong to on that date.  Date should be YYYY-MM-DD. */
		std::vector<Symbol> getAssociatedSectorCodes(ActorId target, const char *date=0, int min_frequency=-1024);

		/** Return a list of the codes of sectors that the given agent belongs
		  * to (based on the ICEWS sector mappings table).  If a date is
		  * specified, then only return sectors that the given target
		  * actor belong to on that date.  Date should be YYYY-MM-DD. */
		std::vector<Symbol> getAssociatedSectorCodes(AgentId target);

		void preCacheActorInfo(const ActorId::HashSet& actorIds);
	private:
		struct Cache;
		Cache *_cache;
	};
	typedef boost::shared_ptr<ActorInfo> ActorInfo_ptr;

} // end namespace ICEWS
#endif
