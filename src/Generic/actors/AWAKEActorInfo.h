// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef KNOWLEDGE_BASE_ACTOR_INFO_H
#define KNOWLEDGE_BASE_ACTOR_INFO_H

#pragma warning( disable: 4266 )

#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorInfo.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>

// Forward declaration
class DatabaseConnection;
typedef boost::shared_ptr<DatabaseConnection> DatabaseConnection_ptr;

class ActorPattern;

/** Class used to look up information about actors.  This information 
* could either be looked up from database tables or from some other
* source (such as files). 
*/
class AWAKEActorInfo : public ActorInfo {
public:
	AWAKEActorInfo();
	~AWAKEActorInfo();

	static boost::shared_ptr<ActorInfo> getAWAKEActorInfo();

	/** Return the name for the given actor (as specified by the
	* table dict_actors.name) */
	virtual std::wstring getActorName(ActorId target);

	/** Return the name for the given sector */
	virtual std::wstring getSectorName(SectorId target);

	/** Return true if the given actor corresponds to a country. */
	virtual bool isACountry(ActorId target);

	/** Return true if the given actor corresponds to an individual. */
	virtual bool isAnIndividual(ActorId target);
	
	/** Return true if the given actor is a GPE */
	virtual bool isAGPE(ActorId target);
	
	/** Return true if the given actor is a Person */
	virtual bool isAPerson(ActorId target);

	/** Return true if the given actor is an Organization */
	virtual bool isAnOrganization(ActorId target);

	/** Return true if the given actor is an Organization */
	virtual bool isAFacility(ActorId target);

	/** BBN actor DB doesn't have CountryID, so return 
	  * ActorId if a country, null CountryId otherwise. */
	virtual CountryId getCountryId(ActorId target);

	/** In the BBN Actor Database, we have more exact entity type
	  * information than for ICEWS, so just return true if GPE 
	  * or LOC. */
	virtual bool mightBeALocation(ActorId target);

	/** No actor/agent codes in BBN Actor dictionary, fake these. */
	virtual Symbol getActorCode(ActorId target);
	virtual Symbol getAgentCode(AgentId target);

	// Not virtual: helper function for others
	boost::shared_ptr<std::vector<ActorId> > getAssociatedActorIds(ActorId target, std::vector<int> link_types, const char *date);

	/** Return a list of actor ids that are linked to the given
	  * actor. If a date is specified, then only return actors 
	  * that are linked to the given target actor
	  * on the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedActorIds(ActorId target, const char *date=0);

	/** Returns a list of actor ids that are linked to the given
	  * actor as a Location. */
	virtual std::vector<ActorId> getAssociatedLocationActorIds(ActorId target, const char *date=0);

	/** Return a list of actor ids for countries that are linked to 
	  * the given actor. If a date is specified, then only return 
	  * countries that are linked to the given target actor on 
	  * the specified date.  Date should be YYYY-MM-DD. */
	virtual std::vector<ActorId> getAssociatedCountryActorIds(ActorId target, const char *date=0);

	/** Return a list of the ids of sectors that the given actor belongs
	* to (based on the sector mappings table).  If a date is
	* specified, then only return sectors that the given target
	* actor belong to on that date.  Date should be YYYY-MM-DD. */
	virtual std::vector<SectorId> getAssociatedSectorIds(ActorId target, const char *date=0);

	/** There's no sector codes in the BBN actor DB, so return
	  * sector name in lieu of code. Date should be YYYY-MM-DD. */
	virtual std::vector<Symbol> getAssociatedSectorCodes(ActorId target, const char *date=0, int min_frequency=-1024);

	/** Since there are no CountryIds in the BBN Actor Database, this
	  * returns the same Ids as getAssociatedCountryActorIds */
	virtual std::vector<CountryId> getAssociatedCountryIds(ActorId target, const char *date=0);

	/** Return the country ISO Code for a particular actor ID */
	virtual Symbol getIsoCodeForActor(ActorId actor_id);

	/** Return the entity type for a particular actor ID */
	virtual Symbol getEntityTypeForActor(ActorId target);

	/** If the database contains actors imported from geonames, that is 
	actors with a geonameid field, then return the ActorId for a 
	particular genonameid. Otherwise return null ActorId */
	virtual ActorId getActorIdForGeonameId(std::wstring &geonameid);

	/** ImportanceScore of actor */
	virtual double getImportanceScoreForActor(ActorId target);

	/** Return the ActorID for an actor, given its canonical name. */
	virtual ActorId getActorByName(std::wstring name);

	/** Return the ActorID for an actor, given its canonical name. */
	virtual AgentId getAgentByName(Symbol name);

	/** Return the human-readable name for the given agent */
	virtual std::wstring getAgentName(AgentId target);

	/** Can agent be paired with a non-country actor? */
	virtual bool isRestrictedToCountryActors(AgentId target);

	/** Return a list of the codes of sectors that the given agent belongs to. */
	virtual std::vector<Symbol> getAssociatedSectorCodes(AgentId target);
	
	/** Used only in ICEWS to assign persons to a default agent category */
	virtual AgentId getDefaultPersonAgentId();
	virtual Symbol getDefaultPersonAgentCode();

	/** Case insensitive check to see if name is country actor name */
	virtual bool isCountryActorName(std::wstring name);

	/** List of patterns loaded from actor database (used only in AWAKEActorInfo **/
	virtual std::vector<ActorPattern *>& getPatterns();

private:
	int _affiliation_link_type_id;
	int _country_link_type_id;
	int _location_link_type_id;

	struct ActorRow;
	struct AgentRow;
	struct Cache;
	Cache *_cache;
	std::map<std::wstring, bool> _countryActorNameCache;

	//LT: removed "Symbol _actorDBName"
	////Symbol _actorDBName;

	std::map<std::wstring, ActorId> _geonameIdToActorIdCache;

	static bool _initialized;
	static boost::shared_ptr<AWAKEActorInfo> _actorInfo;

	void loadActorPatterns();
};
typedef boost::shared_ptr<AWAKEActorInfo> AWAKEActorInfo_ptr;


#endif
