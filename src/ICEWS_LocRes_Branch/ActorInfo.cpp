/// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

// We could potentially do some caching here, if this is a bottleneck.

#include "Generic/common/leak_detection.h"
#include "ICEWS/ActorInfo.h"
#include "ICEWS/ICEWSDB.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <iostream>

namespace ICEWS {

struct DictActorRow {
	DictActorRow(): is_country(false), is_individual(false), is_sector(false) {}
	DictActorRow(const std::wstring &name, bool is_country, bool is_individual, bool is_sector, CountryId country_id, Symbol code)
		: name(name), is_country(is_country), is_individual(is_individual), is_sector(is_sector), country_id(country_id), code(code) {}
	std::wstring name;
	bool is_country; // note: this can be true even if countryId is null!
	bool is_individual;
	bool is_sector;
	CountryId country_id;
	Symbol code;
};

struct DictAgentRow {
	DictAgentRow(): restricted_to_country_actors(false) {}
	DictAgentRow(const std::wstring &name, bool restricted_to_country_actors, Symbol code)
		: name(name), restricted_to_country_actors(restricted_to_country_actors), code(code) {}
	std::wstring name;
	bool restricted_to_country_actors;
	std::vector<Symbol> associatedSectorCodes;
	Symbol code;
};

// We cache the results of database queries about actors, using the actor 
// id and the date for the key.  
struct ActorInfo::Cache {
public:
	// These return refereneces to shared pointers.  The shared pointers will be null if the
	// value hasn't been added to the cache yet; in that case, simply assign a new value
	// to the shared pointer (which is returned by reference).
	boost::shared_ptr<std::vector<ActorId> > &getAssociatedActorIds(ActorId actorId, const char *date) {
		return lookup(associatedActorIds, actorId, date); }
	boost::shared_ptr<std::vector<SectorId> > &getAssociatedSectorIds(ActorId actorId, const char *date) {
		return lookup(associatedSectorIds, actorId, date); }
	boost::shared_ptr<std::vector<Symbol> > &getAssociatedSectorCodes(ActorId actorId, const char *date) {
		return lookup(associatedSectorCodes, actorId, date); }

	Cache(): num_hits(0), num_misses(0) {}
	~Cache() {
		SessionLogger::dbg("ICEWS") << "ActorInfo::Cache: " << num_hits << " hits, " 
			<< num_misses << " misses." << std::endl;
	}

	const DictActorRow& saveDictActorRow(DatabaseConnection::RowIterator &row, DictActorRow& result) const {
		result.name = row.getCellAsWString(0);
		result.is_country = (row.getCellAsInt32(1,0) != 0);
		result.is_individual = (row.getCellAsInt32(2,0) != 0);
		result.is_sector = (row.getCellAsInt32(3,0) != 0);
		result.country_id = (row.isCellNull(4)) ? CountryId() : CountryId(row.getCellAsInt32(4,0));
		result.code = row.getCellAsSymbol(5);
		return result;
	}

	const DictActorRow& getDictActorRow(ActorId target) {
		ActorId::HashMap<DictActorRow>::iterator it = actorRows.find(target);
		if (it != actorRows.end()) {
			return (*it).second;
		} else {
			std::ostringstream query;
			query << "SELECT name, is_country, is_individual, is_sector, country_id, unique_code"
			      << " FROM dict_actors WHERE actor_id=" << target.getId();
			DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
			for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
				if (actorRows.size() >= Cache::MAX_SIZE)
					actorRows.clear();
				return saveDictActorRow(row, actorRows[target]);
			}
			static DictActorRow nullResult(L"UNKNOWN-ACTOR", false, false, false, CountryId(), Symbol());
			return nullResult;
		}
	}

	void loadActorRows(const ActorId::HashSet& actorIds) {
		if (actorIds.size() == 0) return;
		std::ostringstream query;
		query << "SELECT name, is_country, is_individual, is_sector, country_id, actor_id, unique_code"
		      << " FROM dict_actors WHERE actor_id in (";
		size_t num_actors = 0;
		for (ActorId::HashSet::iterator it = actorIds.begin(); it!=actorIds.end(); ++it) {
			if (actorRows.find(*it) == actorRows.end()) {
				if (num_actors>0) query << ", ";
				query << (*it).getId();
				++num_actors;
			}
		}
		if (num_actors==0) return;
		query << ")";
		if (actorRows.size() >= Cache::MAX_SIZE)
			actorRows.clear();
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			ActorId actorId(row.getCellAsInt32(5));
			saveDictActorRow(row, actorRows[actorId]);
		}
	}

	const DictAgentRow& getDictAgentRow(AgentId target) {
		if (agentRows.size()==0) readAgentTable();
		AgentId::HashMap<DictAgentRow>::iterator it = agentRows.find(target);
		if (it != agentRows.end()) {
			return (*it).second;
		} else {
			static DictAgentRow nullResult(L"UNKNOWN-AGENT", false, Symbol(L"UNKNOWN-AGENT"));
			return nullResult;
		}
	}

	AgentId getAgentByCode(Symbol code) {
		if (agentRows.size()==0) readAgentTable();
		Symbol::HashMap<AgentId>::iterator it = agentByCode.find(code);
		if (it == agentByCode.end()) return AgentId();
		else return (*it).second;
	}

	ActorId getActorIdForCountry(CountryId countryId) {
		if (actorIdForCountry.size()==0) readActorIdForCountryTable();
		CountryId::HashMap<ActorId>::iterator it = actorIdForCountry.find(countryId);
		if (it == actorIdForCountry.end()) return ActorId();
		else return (*it).second;
	}

private:
	size_t num_hits;
	size_t num_misses;

	ActorId::HashMap<DictActorRow> actorRows;
	AgentId::HashMap<DictAgentRow> agentRows;
	Symbol::HashMap<AgentId> agentByCode;
	CountryId::HashMap<ActorId> actorIdForCountry;

	// There are only about 200-300 country actors, so it's fine to 
	// keep this table in memory.
	void readActorIdForCountryTable() {
		std::ostringstream query;
		query << "SELECT actor_id, country_id FROM rcdr.dict_actors WHERE country_id IS NOT NULL;";
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			ActorId actorId(row.getCellAsInt32(0));
			CountryId countryId(row.getCellAsInt32(1));
			actorIdForCountry[countryId] = actorId;
		}
	}

	// The agent table contains only around 700 rows total, so we just
	// cache the whole thing in memory.
	void readAgentTable() {
		std::ostringstream query;
		query << "SELECT agent_id, name, restricted_to_country_actors, agent_code FROM dict_agents";
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			AgentId agentId(row.getCellAsInt32(0));
			DictAgentRow& agentRow = agentRows[agentId];
			agentRow.name = row.getCellAsWString(1);
			agentRow.restricted_to_country_actors = (row.getCellAsInt32(2,0) != 0);
			agentRow.code = row.getCellAsSymbol(3);
			agentByCode[agentRow.code] = agentId;
		}
		// Read in the agent/sector mappings as well.
		std::ostringstream sector_query;
		sector_query << "SELECT agent_id, sectors.code FROM dict_agent_sector_mappings"
		      << " JOIN dict_sectors sectors USING(sector_id)";
		for (DatabaseConnection::RowIterator row = icews_db->iter(sector_query); row!=icews_db->end(); ++row) {
			AgentId agentId(row.getCellAsInt32(0));
			agentRows[agentId].associatedSectorCodes.push_back(row.getCellAsSymbol(1));
		}
		// Add an entry for the NULL agent id.
		agentRows[AgentId()] = DictAgentRow(L"UNKNOWN-AGENT", false, Symbol(L"UNKNOWN-AGENT"));
	}

	// We don't bother with anything fancy like a LRU cache; instead, we
	// just flush the cache whenever it reaches MAX_SIZE.
	static const size_t MAX_SIZE = 1000;

	typedef std::pair<ActorId, std::string> KeyType;
	struct HashOp {
		size_t operator()(const KeyType &v) const {
			return static_cast<size_t>(v.first.getId()); }
	};
	struct EqualOp {
		size_t operator()(const KeyType &v1, const KeyType &v2) const {
			return static_cast<size_t>((v1.first==v2.first) && (v1.second==v2.second));}
	};
	template<typename IdType>
	class IdVectorMap: public hash_map<KeyType, boost::shared_ptr<std::vector<IdType> >, HashOp, EqualOp> {};

	IdVectorMap<ActorId> associatedActorIds;
	IdVectorMap<SectorId> associatedSectorIds;
	IdVectorMap<Symbol> associatedSectorCodes;

	template<typename IdType> 
	boost::shared_ptr<std::vector<IdType> > &lookup(IdVectorMap<IdType> &idVectorMap, ActorId target, const char *date) {
		boost::shared_ptr<std::vector<IdType> > &result = idVectorMap[std::make_pair(target, date?date:"")];
		++(result?num_hits:num_misses);
		if ((!result) && (idVectorMap.size() > Cache::MAX_SIZE))
			idVectorMap.clear();
		return result;
	}
};

ActorInfo::ActorInfo(): _cache(0)
{
	_cache = _new ActorInfo::Cache();
}

ActorInfo::~ActorInfo() {
	delete _cache;
}

boost::shared_ptr<ActorInfo> ActorInfo::getActorInfoSingleton() {
	static boost::shared_ptr<ActorInfo> actorInfo = boost::make_shared<ActorInfo>();
	return actorInfo;
}

std::vector<ActorId> ActorInfo::getAssociatedActorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<ActorId> >& result = _cache->getAssociatedActorIds(target, date);
	if (!result) {
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		result = boost::make_shared<std::vector<ActorId> >();
		std::ostringstream query; // p=parent, c=child.
		query << "SELECT DISTINCT p.actor_id FROM dict_actornodes p"
			  << " JOIN dict_actornodes c ON c.actor_id=" << target.getId()
			  << " WHERE p.nsLeft<=c.nsLeft AND p.nsRight >= c.nsRight";
		if (date) 
			query << " AND ((c.min_date is null) or (c.min_date < " << icews_db->toDate(date) << "))"
				  << " AND ((c.max_date is null) or (c.max_date > " << icews_db->toDate(date) << "))";
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			result->push_back(ActorId(row.getCellAsInt32(0)));
		}
	}
	return *result;
}

std::vector<CountryId> ActorInfo::getAssociatedCountryIds(ActorId target, const char *date) {
	std::vector<CountryId> result;
	BOOST_FOREACH(ActorId actorId, getAssociatedActorIds(target, date)) {
		CountryId countryId = getCountryId(actorId);
		if (!countryId.isNull())
			result.push_back(countryId);
	}
	return result;
}

std::vector<ActorId> ActorInfo::getAssociatedCountryActorIds(ActorId target, const char *date) {
	std::vector<ActorId> result;
	BOOST_FOREACH(ActorId actorId, getAssociatedActorIds(target, date)) {
		if (isACountry(actorId))
			result.push_back(actorId);
	}
	return result;
}

std::vector<SectorId> ActorInfo::getAssociatedSectorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<SectorId> >& result = _cache->getAssociatedSectorIds(target, date);
	if (!result) {
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		result = boost::make_shared<std::vector<SectorId> >();
		std::ostringstream query;
		query << "SELECT DISTINCT sector_id FROM dict_sector_mappings "
			  << "WHERE actor_id=" << target.getId();
		if (date) 
			query << " AND ((start_date is null) or (start_date < " << icews_db->toDate(date) << "))"
				  << " AND ((end_date is null) or (end_date > " << icews_db->toDate(date) << "))";
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			result->push_back(SectorId(row.getCellAsInt32(0)));
		}
	}
	return *result;
}

std::vector<Symbol> ActorInfo::getAssociatedSectorCodes(ActorId target, const char *date, int min_frequency) {
	boost::shared_ptr<std::vector<Symbol> >& result = _cache->getAssociatedSectorCodes(target, date);
	if (!result) {
		DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
		result = boost::make_shared<std::vector<Symbol> >();
		std::ostringstream query;
		query << "SELECT DISTINCT s.code FROM dict_sectors s"
			  << " JOIN dict_sector_mappings sm USING(sector_id) "
			  << " WHERE actor_id=" << target.getId();
		if (date) 
			query << " AND ((sm.start_date is null) or (sm.start_date < " << icews_db->toDate(date) << "))"
				  << " AND ((sm.end_date is null) or (sm.end_date > " << icews_db->toDate(date) << "))";
		if (min_frequency > -1024) 
			query << " AND (s.frequency >= " << min_frequency << ")";
		for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
			result->push_back(row.getCellAsSymbol(0));
		}
	}
	return *result;
}

std::wstring ActorInfo::getSectorName(Symbol sectorCode) {
	std::ostringstream query;
	query << "SELECT name from dict_sectors where code=" << DatabaseConnection::quote(sectorCode.to_string());
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		return row.getCellAsWString(0);
	}
	return L"UNKNOWN-SECTOR";
}

std::wstring ActorInfo::getSectorName(SectorId target) {
	std::ostringstream query;
	query << "SELECT name from dict_sectors where sector_id=" << target;
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		return row.getCellAsWString(0);
	}
	return L"UNKNOWN-SECTOR";
}

ActorId ActorInfo::getActorByCode(Symbol actorCode) {
	std::ostringstream query;
	query << "SELECT actor_id from dict_actors where unique_code=" << DatabaseConnection::quote(actorCode.to_string());
	DatabaseConnection_ptr icews_db(getSingletonIcewsDb());
	for (DatabaseConnection::RowIterator row = icews_db->iter(query); row!=icews_db->end(); ++row) {
		return ActorId(row.getCellAsInt32(0));
	}
	return ActorId();
}

std::wstring ActorInfo::getActorName(ActorId target) {
	if (target.isNull()) return L"UNKNOWN-ACTOR";
	return _cache->getDictActorRow(target).name;
}

std::wstring ActorInfo::getAgentName(AgentId target) {
	if (target.isNull()) return L"UNKNOWN-AGENT";
	return _cache->getDictAgentRow(target).name;
}

Symbol ActorInfo::getActorCode(ActorId target) {
	if (target.isNull()) return Symbol(L"UNKNOWN-ACTOR");
	return _cache->getDictActorRow(target).code;
}

Symbol ActorInfo::getAgentCode(AgentId target) {
	if (target.isNull()) return Symbol(L"UNKNOWN-AGENT");
	return _cache->getDictAgentRow(target).code;
}

bool ActorInfo::isACountry(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getDictActorRow(target).is_country;
}

bool ActorInfo::isAnIndividual(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getDictActorRow(target).is_individual;
}

CountryId ActorInfo::getCountryId(ActorId target) {
	if (target.isNull()) return CountryId();
	return _cache->getDictActorRow(target).country_id;
}

bool ActorInfo::isRestrictedToCountryActors(AgentId target) {
	if (target.isNull()) return false;
	return _cache->getDictAgentRow(target).restricted_to_country_actors;
}

std::vector<Symbol> ActorInfo::getAssociatedSectorCodes(AgentId target) {
	return _cache->getDictAgentRow(target).associatedSectorCodes;
}


AgentId ActorInfo::getAgentByCode(Symbol agentCode) {
	return _cache->getAgentByCode(agentCode);
}

ActorId ActorInfo::getActorIdForCountry(CountryId country_id) {
	return _cache->getActorIdForCountry(country_id);
}

void ActorInfo::preCacheActorInfo(const ActorId::HashSet& actorIds) {
	_cache->loadActorRows(actorIds);
}

bool ActorInfo::mightBeALocation(ActorId target) {
	// Business, Entertainment, Multinational Corporation, Media
	static Symbol::SymbolGroup nonLocationSectors = Symbol::makeSymbolGroup(L"BUS 138 214 MED");
	if (isACountry(target)) return true;
	if (isAnIndividual(target)) return false;
	std::vector<Symbol> sectors = getAssociatedSectorCodes(target);
	BOOST_FOREACH(Symbol sector, sectors) {
		if (nonLocationSectors.find(sector) != nonLocationSectors.end())
			return false;
	}
	return true;
}


} // end of namespace ICEWS

