/// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

// We could potentially do some caching here, if this is a bottleneck.

#include "Generic/common/leak_detection.h"

#include "Generic/actors/AWAKEActorInfo.h"
#include "Generic/actors/AWAKEDB.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/ParamReader.h"
#include "Generic/theories/EntityType.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

namespace {
	Symbol PER = Symbol(L"PER");
	Symbol ORG = Symbol(L"ORG");
}

bool AWAKEActorInfo::_initialized = false;
AWAKEActorInfo_ptr AWAKEActorInfo::_actorInfo = AWAKEActorInfo_ptr();

struct AWAKEActorInfo::ActorRow {
	ActorRow() { };
	ActorRow(const std::wstring &canonicalName, Symbol entityType, Symbol entitySubtype, Symbol isoCode, double importance_score)
		: canonicalName(canonicalName), entityType(entityType), entitySubtype(entitySubtype), isoCode(isoCode), importance_score(importance_score) { }

	std::wstring canonicalName;
	Symbol entityType;
	Symbol entitySubtype;
	Symbol isoCode;
	double importance_score;
};

struct AWAKEActorInfo::AgentRow {
	AgentRow() {};
	AgentRow(const std::wstring &name) : name(name){}
	std::wstring name;
	std::vector<Symbol> associatedSectorCodes;
};

struct AWAKEActorInfo::Cache {
public:
	// These return refereneces to shared pointers.  The shared pointers will be null if the
	// value hasn't been added to the cache yet; in that case, simply assign a new value
	// to the shared pointer (which is returned by reference).
	boost::shared_ptr<std::vector<ActorId> > &getAssociatedActorIds(ActorId actorId, const char *date) {
		return lookup(associatedActorIds, actorId, date); }
	boost::shared_ptr<std::vector<ActorId> > &getAssociatedCountryActorIds(ActorId actorId, const char *date) {
		return lookup(associatedCountryActorIds, actorId, date); }
	boost::shared_ptr<std::vector<ActorId> > &getAssociatedLocationActorIds(ActorId actorId, const char *date) {
		return lookup(associatedLocationActorIds, actorId, date); }
	boost::shared_ptr<std::vector<SectorId> > &getAssociatedSectorIds(ActorId actorId, const char *date) {
		return lookup(associatedSectorIds, actorId, date); }
	boost::shared_ptr<std::vector<CountryId> > &getAssociatedCountryIds(ActorId actorId, const char *date) {
		return lookup(associatedCountryIds, actorId, date); }
	boost::shared_ptr<std::vector<Symbol> > &getAssociatedSectorCodes(ActorId actorId, const char *date) {
		return lookup(associatedSectorCodes, actorId, date); }
	
	Cache(): num_hits(0), num_misses(0) {}
	~Cache() {}

	const ActorRow& saveActorRow(DatabaseConnection::RowIterator &row, ActorRow& result) const {
		result.canonicalName = row.getCellAsWString(0);
		result.entityType = row.getCellAsSymbol(1);
		result.entitySubtype = row.getCellAsSymbol(2);
		result.isoCode = row.getCellAsSymbol(3);
		result.importance_score = row.getCellAsDouble(4, 0.0);
		return result;
	}
	
	// LT: don't just automatically search in the default database but look in the database 
	// specified within the ActorId type target
	const ActorRow& getActorRow(ActorId target) {
		ActorId::HashMap<ActorRow>::iterator it = actorRows.find(target);
		if (it != actorRows.end()) {
			return (*it).second;
		} else {
			std::ostringstream query;
			query << "SELECT CanonicalName, EntityType, EntitySubtype, IsoCode, ImportanceScore"
				  << " FROM Actor a LEFT OUTER JOIN ActorIsoCode aic ON a.ActorId=aic.ActorId"
				  << " WHERE a.ActorId=" << target.getId();
			DatabaseConnection_ptr bbn_db(AWAKEDB::getDb(target));
			for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
				if (actorRows.size() >= Cache::MAX_SIZE)
					actorRows.clear();
				return saveActorRow(row, actorRows[target]);
			}
			static ActorRow nullResult(L"UNKNOWN-ACTOR", Symbol(L"unknown"), Symbol(L"unknown"), Symbol(L"unknown"), 0.0);
			return nullResult;
		}
	}
	
	const AgentRow& getAgentRow(AgentId target) {
		if (agentRows.size()==0) readAgentTable();
		AgentId::HashMap<AgentRow>::iterator it = agentRows.find(target);
		if (it != agentRows.end()) {
			return (*it).second;
		} else {
			static AgentRow nullResult(L"UNKNOWN-AGENT");
			return nullResult;
		}
	}

	AgentId getAgentByName(Symbol name) {
		if (agentRows.size()==0) readAgentTable();
		Symbol::HashMap<AgentId>::iterator it = agentByName.find(name);
		if (it != agentByName.end()) {
			return (*it).second;
		} else {
			return AgentId();
		}
	}
	
	AgentId getDefaultPersonAgentId() {
		if (agentRows.size()==0) readAgentTable();
		return defaultPersonAgentId;
	}

	Symbol getDefaultPersonAgentCode() {
		if (agentRows.size()==0) readAgentTable();
		return defaultPersonAgentCode;
	}

private:
	size_t num_hits;
	size_t num_misses;

	ActorId::HashMap<ActorRow> actorRows;
	AgentId::HashMap<AgentRow> agentRows;
	Symbol::HashMap<AgentId> agentByName;

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
	class IdVectorMap: public serif::hash_map<KeyType, boost::shared_ptr<std::vector<IdType> >, HashOp, EqualOp> {};

	IdVectorMap<ActorId> associatedActorIds;
	IdVectorMap<ActorId> associatedCountryActorIds;
	IdVectorMap<ActorId> associatedLocationActorIds;
	IdVectorMap<SectorId> associatedSectorIds;
	IdVectorMap<CountryId> associatedCountryIds;
	IdVectorMap<Symbol> associatedSectorCodes;

	template<typename IdType> 
	boost::shared_ptr<std::vector<IdType> > &lookup(IdVectorMap<IdType> &idVectorMap, ActorId target, const char *date) {
		boost::shared_ptr<std::vector<IdType> > &result = idVectorMap[std::make_pair(target, date?date:"")];
		if ((!result) && (idVectorMap.size() > Cache::MAX_SIZE))
			idVectorMap.clear();
		return result;
	}

	static const size_t MAX_SIZE = 1000;

	AgentId defaultPersonAgentId;
	Symbol defaultPersonAgentCode;

	// LT: iterate through all databases to find the actorDBName
	// remove actorDBName--instead create AgentID using db_name from for loop
	void readAgentTable() {

		defaultPersonAgentId = AgentId();		
		defaultPersonAgentCode = Symbol(L"UNKNOWN-AGENT");

		std::ostringstream query;
		query << "SELECT AgentId, AgentName FROM Agent";
		std::ostringstream sector_query;
		sector_query << "SELECT AgentId, SectorName FROM AgentSectorLink ASL, Sector S WHERE ASL.SectorId = S.SectorId";
		DatabaseConnectionMap dbs = AWAKEDB::getNamedDbs();
		for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
			for (DatabaseConnection::RowIterator row = i->second->iter(query); row!=i->second->end(); ++row) {
				AgentId agentId(row.getCellAsInt32(0), i->first);
				AgentRow& agentRow = agentRows[agentId];
				agentRow.name = row.getCellAsWString(1);	
				agentByName[Symbol(agentRow.name)] = agentId;
				if (agentRow.name == L"Civilian" || agentRow.name == L"Citizen") {
					defaultPersonAgentId = agentId;
					defaultPersonAgentCode = Symbol(agentRow.name);
				}
			}
			// Read in the agent/sector mappings as well.
			// It's OK to use SectorName as the unique "code" here, since the DB constrains this value to be unique
			for (DatabaseConnection::RowIterator row = i->second->iter(sector_query); row!=i->second->end(); ++row) {
				AgentId agentId(row.getCellAsInt32(0), i->first);
				agentRows[agentId].associatedSectorCodes.push_back(row.getCellAsSymbol(1));
			}
		}
		// Add an entry for the NULL agent id.
		agentRows[AgentId()] = AgentRow(L"UNKNOWN-AGENT");
		
		if (defaultPersonAgentId.isNull()) {
			throw UnexpectedInputException("AWAKEActorInfo::readAgentTable", "No default person agent ('Civilian' or 'Citizen') in dictionary");
		}
	}
};

// LT: this is fine to leave as default for now because we probably won't need link table from more than one database
AWAKEActorInfo::AWAKEActorInfo(): ActorInfo(), _affiliation_link_type_id(-1), _country_link_type_id(-1), _location_link_type_id(-1) { 
	DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());

	std::ostringstream query1; 
	query1 << "SELECT LinkTypeId FROM LinkType" 
		   << " WHERE LinkType='Affiliation'";
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query1); row != bbn_db->end(); ++row) 
		_affiliation_link_type_id = row.getCellAsInt32(0);

	std::ostringstream query2;
	query2 << "SELECT LinkTypeId FROM LinkType" 
		   << " WHERE LinkType='Country'";
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query2); row != bbn_db->end(); ++row) 
		_country_link_type_id = row.getCellAsInt32(0);
	
	std::ostringstream query3;
	query3 << "SELECT LinkTypeId FROM LinkType" 
		   << " WHERE LinkType='Location'";
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query3); row != bbn_db->end(); ++row) 
		_location_link_type_id = row.getCellAsInt32(0);

	//LT: removing references to _actorDBName
	//_actorDBName = ParamReader::getParam(Symbol(L"bbn_actor_db_name"));
	//if (_actorDBName.is_null())
	//	_actorDBName = Symbol(L"default");

	_cache = _new AWAKEActorInfo::Cache();

	loadActorPatterns();
}

ActorInfo_ptr AWAKEActorInfo::getAWAKEActorInfo() {
	if (!_initialized) {
		_actorInfo = boost::make_shared<AWAKEActorInfo>();
		_initialized = true;
	}
	return _actorInfo;
}

AWAKEActorInfo::~AWAKEActorInfo() { 
	delete _cache;
};

CountryId AWAKEActorInfo::getCountryId(ActorId target) {
	if (!isACountry(target))
		return CountryId();
	
	return CountryId(target.getId(), target.getDbName());
}

bool AWAKEActorInfo::mightBeALocation(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getGPEType().getName() ||
		   _cache->getActorRow(target).entityType == EntityType::getLOCType().getName();
}

Symbol AWAKEActorInfo::getActorCode(ActorId target) {
	std::wstringstream str;
	str << target.getId();
	return Symbol(str.str());
}

Symbol AWAKEActorInfo::getAgentCode(AgentId target) {
	std::wstringstream str;
	str << target.getId();
	return Symbol(str.str());
}

boost::shared_ptr<std::vector<ActorId> > AWAKEActorInfo::getAssociatedActorIds(ActorId target, std::vector<int> link_types, const char *date) {
	boost::shared_ptr<std::vector<ActorId> > result = boost::make_shared<std::vector<ActorId> >();

	// LT: replaced getDefaultDb with getDb(target)
	DatabaseConnection_ptr bbn_db(AWAKEDB::getDb(target));
	std::ostringstream query;

	query << "SELECT DISTINCT RightActorId FROM ActorLink" 
		<< " WHERE LeftActorId=" << target.getId();
	if (date) 
		query << " AND ((StartDate is null) or (StartDate < " << bbn_db->toDate(date) << "))"
		<< " AND ((EndDate is null) or (EndDate > " << bbn_db->toDate(date) << "))";
	if (link_types.size() > 0) {
		query << " AND LinkTypeId in (";
		for (std::vector<int>::iterator iter = link_types.begin(); iter != link_types.end(); iter++) {
			if (iter != link_types.begin())
				query << ", ";
			query << (*iter);
		}
		query << ")";
	}

	for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row!=bbn_db->end(); ++row) {
		result->push_back(ActorId(row.getCellAsInt32(0), target.getDbName()));
	}

	return result;
}

std::vector<ActorId> AWAKEActorInfo::getAssociatedActorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<ActorId> >& result = _cache->getAssociatedActorIds(target, date);

	if (!result) {
		std::vector<int> link_types;
		result = getAssociatedActorIds(target, link_types, date); // This adds to the cache
	} 

	return (*result);
}

std::vector<ActorId> AWAKEActorInfo::getAssociatedCountryActorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<ActorId> >& result = _cache->getAssociatedCountryActorIds(target, date);

	if (!result) {
		std::vector<int> link_types;
		link_types.push_back(_country_link_type_id);
	    result = getAssociatedActorIds(target, link_types, date); // This adds to the cache
	} 

	return (*result);
}

std::vector<ActorId> AWAKEActorInfo::getAssociatedLocationActorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<ActorId> >& result = _cache->getAssociatedLocationActorIds(target, date);

	if (!result) {
		std::vector<int> link_types;
		link_types.push_back(_location_link_type_id);
		result = getAssociatedActorIds(target, link_types, date); // This adds to the cache
	} 

	return (*result);
}

std::vector<CountryId> AWAKEActorInfo::getAssociatedCountryIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<CountryId> >& result = _cache->getAssociatedCountryIds(target, date);

	if (!result) {
		result = boost::make_shared<std::vector<CountryId> >();

	    std::vector<ActorId> actorIds = getAssociatedCountryActorIds(target, date);
		BOOST_FOREACH(ActorId aid, actorIds) {
			result->push_back(CountryId(aid.getId(), aid.getDbName()));
		}
	}

	return *result;
}

std::vector<SectorId> AWAKEActorInfo::getAssociatedSectorIds(ActorId target, const char *date) {
	boost::shared_ptr<std::vector<SectorId> >& result = _cache->getAssociatedSectorIds(target, date);

	if (!result) {
		// LT: replaced getDefaultDb with getDb(target)
		DatabaseConnection_ptr bbn_db(AWAKEDB::getDb(target));
		result = boost::make_shared<std::vector<SectorId> >();
		std::ostringstream query;

		query << "SELECT DISTINCT SectorId FROM ActorSectorLink"
			  << " WHERE ActorId=" << target.getId();
		if (date) 
			query << " AND ((StartDate is null) or (StartDate < " << bbn_db->toDate(date) << "))"
			      << " AND ((EndDate is null) or (EndDate > " << bbn_db->toDate(date) << "))";

		for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
			result->push_back(SectorId(row.getCellAsInt32(0), target.getDbName()));
		}
	}
	return *result;
}

std::vector<Symbol> AWAKEActorInfo::getAssociatedSectorCodes(ActorId target, const char *date, int min_frequency) {
	boost::shared_ptr<std::vector<Symbol> >& result = _cache->getAssociatedSectorCodes(target, date);

	if (!result) {
		// LT: replaced getDefaultDb with getDb(target)
		DatabaseConnection_ptr bbn_db(AWAKEDB::getDb(target));
		result = boost::make_shared<std::vector<Symbol> >();
		std::vector<int> idResults;
		std::ostringstream query;

		query << "SELECT DISTINCT s.SectorName, s.SectorId FROM ActorSectorLink asl, Sector s"
			  << " WHERE ActorId=" << target.getId()
			  << " AND asl.SectorId=s.SectorId";
		if (date) 
			query << " AND ((asl.StartDate is null) or (asl.StartDate < " << bbn_db->toDate(date) << "))"
			<< " AND ((asl.EndDate is null) or (asl.EndDate > " << bbn_db->toDate(date) << "))";

		for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
			result->push_back(row.getCellAsSymbol(0));
			idResults.push_back(row.getCellAsInt32(1));
		}

		// Get parent sectors as well, (and parents of parents, etc.)
		BOOST_FOREACH(int id, idResults) {
			while (id != -1) {
				std::ostringstream parentSectorQuery;
				parentSectorQuery
					<< "SELECT s2.SectorName, s2.SectorId FROM Sector s1, Sector s2"
					<< " WHERE s1.SectorId=" << id
					<< " AND s1.ParentSectorId=s2.SectorId";
				id = -1;
				for (DatabaseConnection::RowIterator row = bbn_db->iter(parentSectorQuery); row != bbn_db->end(); ++row) {
					Symbol parentSectorName = row.getCellAsSymbol(0);
					if (std::find(result->begin(), result->end(), parentSectorName) != result->end())
						result->push_back(parentSectorName);
					id = row.getCellAsInt32(1);
				}
			} 
		}
	}
	return *result;
}

std::wstring AWAKEActorInfo::getSectorName(SectorId target) {
	std::ostringstream query;
	query << "SELECT SectorName from Sector where SectorId=" << target;
	// LT: replaced getDefaultDb with getDb(target)
	DatabaseConnection_ptr bbn_db(AWAKEDB::getDb(target));
	for (DatabaseConnection::RowIterator row = bbn_db->iter(query); row != bbn_db->end(); ++row) {
		return row.getCellAsWString(0);
	}
	return L"UNKNOWN-SECTOR";
}

std::wstring AWAKEActorInfo::getActorName(ActorId target) {
	if (target.isNull()) return L"UNKNOWN-ACTOR";
	return _cache->getActorRow(target).canonicalName;
}

Symbol AWAKEActorInfo::getIsoCodeForActor(ActorId target) {
	if (target.isNull()) return Symbol(L"unknown");
	return _cache->getActorRow(target).isoCode;
}

Symbol AWAKEActorInfo::getEntityTypeForActor(ActorId target) {
	if (target.isNull()) return Symbol();
	return _cache->getActorRow(target).entityType;
}

bool AWAKEActorInfo::isACountry(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getGPEType().getName() &&
		   _cache->getActorRow(target).entitySubtype == Symbol(L"Nation");
}

bool AWAKEActorInfo::isAnIndividual(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType ==  EntityType::getPERType().getName();
}

bool AWAKEActorInfo::isAGPE(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getGPEType().getName();
}

bool AWAKEActorInfo::isAPerson(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getPERType().getName();
}

bool AWAKEActorInfo::isAnOrganization(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getORGType().getName();
}

bool AWAKEActorInfo::isAFacility(ActorId target) {
	if (target.isNull()) return false;
	return _cache->getActorRow(target).entityType == EntityType::getFACType().getName();
}

void AWAKEActorInfo::loadActorPatterns() {
	double min_confidence = ParamReader::getOptionalFloatParamWithDefaultValue("minimum_actor_string_confidence", 0);
	//LT: changed to be looping over all dbs instead of just getting default
	//DatabaseConnection_ptr bbn_db = AWAKEDB::getDefaultDb();
	DatabaseConnectionMap dbs = AWAKEDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		double min_per_actor_importance_score = 
			ParamReader::getOptionalFloatParamWithDefaultValue("min_per_actor_importance_score", 0.0);
		double min_org_actor_importance_score = 
			ParamReader::getOptionalFloatParamWithDefaultValue("min_org_actor_importance_score", 0.0);

		std::vector<std::string> allowedSources;
		std::vector<std::string> disallowedSources;
		if (ParamReader::hasParam("actor_string_allowed_sources") && ParamReader::hasParam("actor_string_disallowed_sources")) {
			throw UnexpectedInputException("AWAKEActorInfo::loadActorPatterns", "Only one of actor_string_allowed_sources or actor_string_disallowed_sources may be specified");
		}
		if (ParamReader::hasParam("actor_string_allowed_sources")) {
			allowedSources = ParamReader::getStringVectorParam("actor_string_allowed_sources");
		} else if (ParamReader::hasParam("actor_string_disallowed_sources")) {
			disallowedSources = ParamReader::getStringVectorParam("actor_string_disallowed_sources");
		}

		std::ostringstream query;
		query << "SELECT a.ActorId, a.EntityType, s.ActorStringId, s.String, s.Acronym, s.Confidence"
			  << ", s.RequiresContext, a.ImportanceScore";

		// Maintain backwards compatibility with databases that don't have an ActorStringSource table
		if (allowedSources.size() != 0 || disallowedSources.size() != 0)
			query << ", asr.OriginalSourceElement";

		query << " FROM Actor a, ActorString s";

		if (allowedSources.size() != 0 || disallowedSources.size() != 0)
			query << ", ActorStringSource asr";

		query << " WHERE a.ActorId = s.ActorId";
		
		if (allowedSources.size() != 0 || disallowedSources.size() != 0)
			query << " AND s.ActorStringId = asr.ActorStringId";

		if (min_confidence > 0)
			query << " AND s.Confidence > " << min_confidence;

		bool printed = false;
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row != i->second->end(); ++row) {
			ActorPattern *ap = _new ActorPattern();
			Symbol entityType = Symbol(row.getCellAsWString(1));
			double importance_score = row.getCellAsDouble(7);
		
			if (allowedSources.size() != 0 || disallowedSources.size() != 0) {
				std::string source = row.getCellAsString(8);
				std::vector<std::string> stringSources;
				boost::split(stringSources, source, boost::is_any_of(" ,"));
				if (allowedSources.size() != 0) {
					bool found_match = false;
					BOOST_FOREACH(std::string as, allowedSources) {
						if (std::find(stringSources.begin(), stringSources.end(), as) != stringSources.end()) {
							found_match = true;
						}
					}
					if (!found_match)
						continue;
				}
				if (disallowedSources.size() != 0) {
					bool found_match = false;
					BOOST_FOREACH(std::string as, disallowedSources) {
						if (std::find(stringSources.begin(), stringSources.end(), as) != stringSources.end()) {
							found_match = true;
						}
					}
					if (found_match)
						continue;
				}
			}
			if (min_per_actor_importance_score > 0.0 && entityType == PER && importance_score < min_per_actor_importance_score)
				continue;
			if (min_org_actor_importance_score > 0.0 && entityType == ORG && importance_score < min_org_actor_importance_score)
				continue;
			// LT: replaced _actorDBName with i->first
			ap->actor_id = ActorId(row.getCellAsInt32(0), i->first);
			ap->entityTypeSymbol = entityType;
			ap->pattern_id = ActorPatternId(row.getCellAsInt32(2), i->first);

			std::wstring str = row.getCellAsWString(3);
			std::vector<std::wstring> words;
			boost::split(words, str, boost::is_any_of(" "));
			ap->pattern = std::vector<Symbol>();
			BOOST_FOREACH(std::wstring word, words) {
				ap->pattern.push_back(Symbol(word));
				std::transform(word.begin(), word.end(), word.begin(), towlower);
				ap->lcPattern.push_back(Symbol(word));
			}
			
			int acronym_int = row.getCellAsBool(4);
			if (acronym_int)
				ap->acronym = true;
			else
				ap->acronym = false;

			std::string confidenceStr = row.getCellAsString(5);
			ap->confidence = (float)::atof(confidenceStr.c_str());
			
			ap->requires_context = false;
			int context_int = row.getCellAsBool(6);
			if (context_int)
				ap->requires_context = true;
			else
				ap->requires_context = false;

			ap->lcString = ActorPattern::getNameFromSymbolList(ap->lcPattern);
					
			_patterns.push_back(ap); 
		}
	}
}

ActorId AWAKEActorInfo::getActorIdForGeonameId(std::wstring &geonameid) {
	std::map<std::wstring, ActorId>::iterator iter = _geonameIdToActorIdCache.find(geonameid);
	if (iter != _geonameIdToActorIdCache.end())
		return (*iter).second;
	
	std::ostringstream query;
	query << "SELECT ActorId"
		  << " FROM Actor WHERE GeonameId=" << DatabaseConnection::quote(geonameid);
	//LT: changed to be looping over all dbs instead of just getting default
	//DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
	DatabaseConnectionMap dbs = AWAKEDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row != i->second->end(); ++row) {
			// LT: replaced _actorDBName with i->first
			ActorId actor_id(row.getCellAsInt32(0), i->first);
			_geonameIdToActorIdCache[geonameid] = actor_id;
			return actor_id;
		}
	}
	_geonameIdToActorIdCache[geonameid] = ActorId();
	return ActorId();
}

double AWAKEActorInfo::getImportanceScoreForActor(ActorId target) {
	if (target.isNull()) return 0.0;
	return _cache->getActorRow(target).importance_score;
}

ActorId AWAKEActorInfo::getActorByName(std::wstring name) {
	// Inefficient, but is currently only used to get the United States actor
	std::ostringstream query;
	query << "SELECT ActorId"
		  << " FROM Actor WHERE CanonicalName=" << DatabaseConnection::quote(name);
	//LT: changed to be looping over all dbs instead of just getting default
	//DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
	DatabaseConnectionMap dbs = AWAKEDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row != i->second->end(); ++row) {
			// LT: replaced _actorDBName with i->first
			ActorId actor_id(row.getCellAsInt32(0), i->first);
			return actor_id;
		}
	}
	return ActorId();
}

AgentId AWAKEActorInfo::getAgentByName(Symbol name) {
	return _cache->getAgentByName(name);
}

std::wstring AWAKEActorInfo::getAgentName(AgentId target) {	
	return _cache->getAgentRow(target).name;
}

bool AWAKEActorInfo::isRestrictedToCountryActors(AgentId target) {
	// Always false for AWAKE agents
	return false;
}

std::vector<Symbol> AWAKEActorInfo::getAssociatedSectorCodes(AgentId target) {
	return _cache->getAgentRow(target).associatedSectorCodes;
}

AgentId AWAKEActorInfo::getDefaultPersonAgentId() {
	return _cache->getDefaultPersonAgentId();
}

Symbol AWAKEActorInfo::getDefaultPersonAgentCode() {
	return _cache->getDefaultPersonAgentCode();
}

bool AWAKEActorInfo::isCountryActorName(std::wstring name) {
	std::transform(name.begin(), name.end(), name.begin(), towlower);

	if (_countryActorNameCache.find(name) != _countryActorNameCache.end())
		return _countryActorNameCache[name];

	std::ostringstream query;
	query << "SELECT ActorId"
		  << " FROM Actor WHERE EntitySubtype='Nation'"
		  << " AND lower(CanonicalName)=" << DatabaseConnection::quote(name);

	//LT: changed to be looping over all dbs instead of just getting default
	//DatabaseConnection_ptr bbn_db(AWAKEDB::getDefaultDb());
	DatabaseConnectionMap dbs = AWAKEDB::getNamedDbs();
	for (DatabaseConnectionMap::iterator i = dbs.begin(); i != dbs.end(); ++i) {
		for (DatabaseConnection::RowIterator row = i->second->iter(query); row != i->second->end(); ++row) {
			_countryActorNameCache[name] = true;
			return true;
		}
	}

	_countryActorNameCache[name] = false;
	return false;
}

std::vector<ActorPattern *>& AWAKEActorInfo::getPatterns() {
	return _patterns;
}
