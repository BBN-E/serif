/**
 *
 **/

#pragma once
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <math.h>
#include "boost/foreach.hpp"
#include "Generic/sqlite/SqliteDB.h"
#include "boost/shared_ptr.hpp"
#include "PredFinder/elf/ElfRelation.h"
#include "PredFinder/elf/ElfRelationArg.h"
#include "PredFinder/elf/ElfIndividual.h"

class EntitySet;

#define DEGREES_TO_RADIANS 57.2957795130823208767981548141
#define CONST_E 2.7182818284590452353602874713
#define NEAREST_N 3
#define TOPN_W .5 //balance between avg distance and top n average distance
#define DISTANCE_CENTER 200.0		//average distance at while distance stops being very relevant (miles)
//calculated from: 25 miles relevance should be .99
#define DISTANCE_FITTING -0.0262578	//parameter on the sigmoid curve to capture variaton in DISTANCE_CENTER
#define POPULATION_CENTER 60000		//average population at which population starts being relevant
//calculated from: 50,000 variation area
#define POPULATION_FITTING 0.0001	//parameter on the sigmoid curve to capture variaton in POPULATION_CENTER
#define POPULATION_TO_DISTANCE_RATIO 3 //population is X times more important than distance
#define DEFAULT_POP 5000			
#define NATION_BOOST 250			//being in the same nation makes you X miles closer than reality
#define STATE_BOOST 100				//being in the same state makes you X miles closer than reality


struct LocationData {
	std::wstring URI;
	std::wstring StateURI;
	std::wstring NationURI;
	int Population;
	bool Default;
	double Latitude;
	double Longitude;
	int MentionCount;
};

typedef std::vector<LocationData> Locations;
typedef std::map<int,Locations> EntityMapType;

class LocationDB {
public:
	LocationDB(const std::string dbLocation);
	~LocationDB(void);
	
	void addEntity(int entityId, const std::set<std::wstring> &aliases, const std::set<std::wstring> &relation, int mentionCount, bool relationParent=false);
	void disambiguate(const EntitySet* eSet);
	std::wstring getEntityUri(int entityId) const;
	bool isValidIfNationState(const EntitySet* entities, const Entity* entity) const;
	std::set<ElfRelation_ptr> getURIGazetteerRelations(const DocTheory* doc_theory, ElfIndividual_ptr ind, std::wstring domain_prefix);
	void clear() const;

	int subgpe_good;
	int subgpe_bad;
	int lookup_ambig;
	int lookup_unambig;
	int by_default;
	int by_country;
	int by_pop;
	int total;

private:
	SqliteDB *db;
	EntityMapType *entityMap;
	std::set<std::wstring> *banList;

	void addSqlTable(const Table_ptr uriData, Locations &aliasVector, int mentionCount) const;
	Locations easyDisambiguate(Locations aliasVector);
	double getScore(const LocationData location) const;
	double locationDistance(const LocationData loc1, const LocationData loc2) const;

	int getType(LocationData target);
	ElfRelation_ptr getDirectRelationArg(std::wstring typeName, std::wstring domain_prefix, std::wstring URI, ElfIndividual_ptr ind);
	ElfRelation_ptr getIndirectRelationArg(std::wstring typeName1, std::wstring typeName2, std::wstring URI1, std::wstring URI2, 
												  std::wstring domain_prefix, ElfIndividual_ptr ind);
	std::vector<ElfRelationArg_ptr> getRelArgPair(ElfRelationArg* arg1,ElfRelationArg* arg2);

};

typedef boost::shared_ptr<LocationDB> LocationDB_ptr;
