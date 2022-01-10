// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

// Filename: PGDatabaseManager.h

#ifndef PG_DATABASE_MANAGER_H
#define PG_DATABASE_MANAGER_H

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"
#include <vector>

#include "Generic/database/DatabaseConnection.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/PGFact.h"

#include "Generic/common/bsp_declare.h"
BSP_DECLARE(PGDatabaseManager);
BSP_DECLARE(PGActorInfo);
BSP_DECLARE(PGFactArgument);
BSP_DECLARE(PGFactDate);
BSP_DECLARE(GenericHypothesis);

/*! \brief A simple exception class for reporting PGDatabaseManager errors
*/
class PGDatabaseManagerException : public std::runtime_error {
public: explicit PGDatabaseManagerException( const char * s ) : std::runtime_error( s ) {}
};

/*! \brief The PGDatabaseManager class is responsible for pulling data from the 
    fact database. (This database is produced by FactFinder.exe and 
    FactUploader.exe)
*/
class PGDatabaseManager
{
public:
	// Allow boost access to the private constructor
	friend PGDatabaseManager_ptr boost::make_shared<PGDatabaseManager>(std::string const&);
	
	// Destructor
	~PGDatabaseManager();

	// Get results of query profiling (if turned on)
	std::string getProfileResults();

	// Clear actor string cache
	void clearActorStringCache();
	 
	// Return a list of (document) Facts that involve the given actor and have the specified fact types
	std::vector<PGFact_ptr> getFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids);
	std::vector<PGFact_ptr> getFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names);

	// Returns a list of Fact objects representing the KBFacts for these actors
	std::vector<PGFact_ptr> getKBFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names, bool ignore_status);
	std::vector<PGFact_ptr> getKBFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids, bool ignore_status);

	// Returns a list of Fact objects representing the external facts for these actors
	std::vector<PGFact_ptr> getExternalFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names);
	std::vector<PGFact_ptr> getExternalFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids);

	// Actual function that does the retrieval for KB or external facts
	std::vector<PGFact_ptr> getKBOrExternalFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids, std::string prefix, bool ignore_status);

	// Functions that interact primarily with actor tables
	std::vector<PGActorInfo_ptr> getPossibleActorsForName(std::wstring actor_name);
	PGActorInfo_ptr getBestActorForName(std::wstring actor_name, Profile::profile_type_t profileType = Profile::UNKNOWN);
	PGActorInfo_ptr getActorInfoForId(int actor_id);
	typedef std::map<std::wstring, double> actor_confidence_map_t;	
	actor_confidence_map_t getActorStringsForActor(int actor_id);
	std::vector<std::wstring> getDocumentCanonicalNamesForActor(int actor_id);

	// Functions that interact primarily with KB facts
	std::set<int> getReliableEmployees(int employer_id);

	// Functions that interact with profile-ish tables 
	void updateSlotInfo();
	void outDateProfile(Profile_ptr profile);
	bool isUpToDate(int actor_id);
	bool uploadProfile(Profile_ptr profile, ProfileConfidence_ptr confidenceModel, bool print_to_screen = false);

	// Functions relevant for EPS-style operation
	std::set<int> getActorsInEpoch(int epoch_id, int min_count, std::string entity_type);
	std::set<int> getRequestedActors();	

	// HELPERS for retrieving and caching various id <--> string conversions
	int convertStringToId(std::string table_name, std::string str, bool allow_new);
	std::vector<int> getFactTypeIds(std::vector<std::string>& fact_type_names);
		
private:
	// Set up fact retriever to retrieve facts from a db
	PGDatabaseManager(std::string conn_str);
	DatabaseConnection_ptr _db;
	int _epoch_id;
	int _serif_extractor_id;

	// For avoiding repeated databse queries
	std::map<int, actor_confidence_map_t> _actorStringCache;

	// When running for debugging only
	bool _skip_database_upload;

	std::vector<int> _externalFactSourceIds;

	size_t _max_facts_per_fact_type;
	typedef std::map<boost::gregorian::date, std::list<int> > date_fact_map_t;

	bool _fake_kb_fact_creation_time;

	static const float MIN_ACTOR_MATCH_CONFIDENCE;

	// DB Helpers
	int insertReturningInt(std::wstring insertQuery, std::wstring returnField);
	int insertReturningInt(std::string insertQuery, std::string returnField);

	// HELPERS for retrieving and caching various id <--> string conversions
	std::string convertIdToString(std::string table_name, int id);
	std::map< std::pair <std::string, int>, std::string > _idToString;
	std::map< std::pair <std::string, std::string>, int > _stringToId;
	int getDisplaySlotId(ProfileSlot_ptr slot);
	std::string getDBDate(boost::gregorian::date& d);
		
	// UPLOAD helpers
	void uploadProfileSlot(Profile_ptr profile, ProfileSlot_ptr slot, std::set<int>& usedKBFactIds, 
		std::vector<PGFact_ptr>& existingKBFacts, std::map<int,int>& existingProfileFacts, std::map<int,int>& existingBinaryRelations);
	int uploadHypothesis(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, std::vector<PGFact_ptr>& existingKBFacts,
										  std::map<int, int>& existingProfileFacts, std::map<int, int>& existingBinaryRelations);
	int uploadDisplayItem(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth);
	void markActorProfileUpdated(int actor_id);
	void cleanUpProfileInformation(std::set<int>& usedKBFacts, std::vector<PGFact_ptr>& existingKBFacts,
										  std::map<int, int>& existingProfileFacts, std::map<int, int>& existingBinaryRelations);
	void cleanUpCacheTable(std::set<int>& factsToPrune, std::map<int, int>& existingCacheFacts, std::string table_name);

	// Get ProfileFacts or BinaryRelations for a particular actor(s)
	std::map<int,int> getBinaryRelations(std::vector<int>& actor_ids);
	std::map<int,int> getProfileFacts(std::vector<int>& actor_ids);
	
	// KB Facts
	PGFact_ptr findMatchingKBFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, std::vector<PGFact_ptr>& existingKBFacts);
	void updateExistingKBFact(int kb_fact_id, double confidence, int fact_status_id = -1, GenericHypothesis_ptr hypoth = GenericHypothesis_ptr());
	void markCacheEntryValid(std::string table_name, int item_id);
	int createKBFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth);
	int createProfileFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, int kb_fact_id);
	int createBinaryRelation(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, int kb_fact_id);
	void addKBArgument(int kb_fact_id, GenericHypothesis::kb_arg_t kb_arg);	
	void addSupportToKBFact(int kb_fact_id, GenericHypothesis_ptr hypoth);

	// Fact status
	std::map<int, PGFact::status_type_t> _factStatusById;	
	int _valid_fact_status_id;
	int _untrusted_fact_status_id;
	
};

#endif
