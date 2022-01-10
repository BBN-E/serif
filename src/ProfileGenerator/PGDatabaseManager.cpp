// Copyright (c) 2010 by BBNT Solutions LLC
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/OutputUtil.h"

#include "ProfileGenerator/PGDatabaseManager.h"
#include "ProfileGenerator/PGFact.h"
#include "ProfileGenerator/PGActorInfo.h"
#include "ProfileGenerator/Profile.h"
#include "ProfileGenerator/ProfileSlot.h"
#include "ProfileGenerator/GenericHypothesis.h"
#include "ProfileGenerator/NameHypothesis.h"
#include "ProfileGenerator/DescriptionHypothesis.h"
#include "ProfileGenerator/EmploymentHypothesis.h"
#include "ProfileGenerator/DatabaseFactDate.h"
#include "ProfileGenerator/DateDescription.h"

#include "boost/foreach.hpp"
#include <boost/algorithm/string.hpp>

#include <vector>
#include <ctime>
#include <sstream>
#include <string>
#include <map>

const float PGDatabaseManager::MIN_ACTOR_MATCH_CONFIDENCE = 0.89F;

PGDatabaseManager::PGDatabaseManager(std::string conn_str) {

	_db = DatabaseConnection::connect(conn_str);
	if (_db->getSqlVariant() != "postgresql")
		throw UnexpectedInputException("PGDatabaseManager::PGDatabaseManager", "ProfileGenerator only currently works with postgres");

	if (ParamReader::isParamTrue("profile_pg_database_queries"))
		_db->enableProfiling();

	_epoch_id = ParamReader::getOptionalIntParamWithDefaultValue("epoch", 0);
	_fake_kb_fact_creation_time = ParamReader::getRequiredTrueFalseParam("fake_kb_fact_creation_time");
	_skip_database_upload = ParamReader::isParamTrue("skip_database_upload");
	_max_facts_per_fact_type = ParamReader::getOptionalIntParamWithDefaultValue("max_facts_per_fact_type", 0);

	std::vector<std::string> external_fact_source_ids;
	if (ParamReader::hasParam("external_fact_sources")) {
		std::string param = ParamReader::getRequiredParam("external_fact_sources");
		boost::split(external_fact_source_ids, param, boost::is_any_of(";"));
		BOOST_FOREACH(std::string source, external_fact_source_ids) {
			int id = convertStringToId("Source", source, false);
			if (id != -1)
				_externalFactSourceIds.push_back(id);
		}
	}

	_serif_extractor_id = convertStringToId("Extractor", "BBN_SERIF", false);

	// Initialize _factStatusById and _valid_fact_status_id
	_valid_fact_status_id = -1;
	_untrusted_fact_status_id = -1;
	std::stringstream queryStream;
	queryStream << "SELECT KBFactStatusId, KBFactStatusName from KBFactStatus";
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		int id = row.getCellAsInt32(0);
		std::string name = row.getCellAsString(1);
		if (name == "UserDeleted")
			_factStatusById[id] = PGFact::USER_DELETED;
		else if (name == "Untrusted") {
			_factStatusById[id] = PGFact::UNTRUSTED;
			_untrusted_fact_status_id = id;
		} else if (name == "Trimmed")
			_factStatusById[id] = PGFact::TRIMMED;
		else if (name == "Valid") {
			_factStatusById[id] = PGFact::VALID;
			_valid_fact_status_id = id;
		} else if (name == "UserAdded") 
			_factStatusById[id] = PGFact::USER_ADDED;
		else {
			std::stringstream err;
			err << "Unknown status in KBFactStatus: " << name << std::endl;
			SessionLogger::err("PG") << err.str();
			throw PGDatabaseManagerException(err.str().c_str());
		}
	}
	if (_valid_fact_status_id == -1) {
		std::stringstream err;
		err << "No status \"Valid\" found in KBFactStatus table" << std::endl;
		SessionLogger::err("PG") << err.str();
		throw PGDatabaseManagerException(err.str().c_str());
	}	
	if (_untrusted_fact_status_id == -1) {
		std::stringstream err;
		err << "No status \"Untrusted\" found in KBFactStatus table" << std::endl;
		SessionLogger::err("PG") << err.str();
		throw PGDatabaseManagerException(err.str().c_str());
	}
		
}

PGDatabaseManager::~PGDatabaseManager() {}

std::string PGDatabaseManager::getProfileResults() {
	return _db->getProfileResults();
}

// Database convenience function. Here because this ONLY WORKS WITH POSTGRESSQL
int PGDatabaseManager::insertReturningInt(std::wstring insertQuery, std::wstring returnField) {

	std::wstringstream insertStream;
	insertStream << insertQuery << L" RETURNING " << returnField;

	int ret_val = -1;
	for (DatabaseConnection::RowIterator row = _db->iter(insertStream.str().c_str()); row!=_db->end(); ++row) {	
		// There should only be one of these, but this is the syntax I'm familiar with	
		ret_val = row.getCellAsInt32(0);
		// I'm not breaking because there was some weird behavior breaking out of these iterators once
	}
	return ret_val;
}

// Database convenience function. Here because this ONLY WORKS WITH POSTGRESSQL
// Exactly as above, but with strings instead of wstrings
int PGDatabaseManager::insertReturningInt(std::string insertQuery, std::string returnField) {
	std::stringstream insertStream;
	insertStream << insertQuery << " RETURNING " << returnField;
	int ret_val = -1;

	for (DatabaseConnection::RowIterator row = _db->iter(insertStream.str().c_str()); row!=_db->end(); ++row) {	
		ret_val = row.getCellAsInt32(0);
	}
	return ret_val;
}

//
// Returns a list of Fact objects represenging the document facts for these actors, of this fact type
//
std::vector<PGFact_ptr> PGDatabaseManager::getFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names) {

	std::vector<PGFact_ptr> results;	
	if (actor_ids.size() == 0)
		return results;

	std::vector<int> fact_type_ids;
	BOOST_FOREACH(std::string fact_type_name, fact_type_names) {
		int fact_type_id = convertStringToId("FactType", fact_type_name, false);
		if (fact_type_id == -1) {
			SessionLogger::dbg("PG") << "failed to find a fact type named '" << fact_type_name.c_str() << "' in the _db" << std::endl;
			continue;
		}	
		fact_type_ids.push_back(fact_type_id);
	}

	if (fact_type_ids.size() == 0)
		return results;
	
	return getFacts(actor_ids, fact_type_ids);
}

//
// Returns a list of Fact objects representing the document facts for these actors, of this fact type
//
std::vector<PGFact_ptr> PGDatabaseManager::getFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids) {
	
	std::vector<PGFact_ptr> results;		
	if (actor_ids.size() == 0 || fact_type_ids.size() == 0)
		return results;
	
	// Go get facts from external sources if they exist
	if (_externalFactSourceIds.size() != 0) {
		std::vector<PGFact_ptr> externalFacts = getExternalFacts(actor_ids, fact_type_ids);
		BOOST_FOREACH(PGFact_ptr fact, externalFacts) {
			results.push_back(fact);
		}
	}
	
	std::stringstream queryStream;
	queryStream << "SELECT MFA.FactId, F.DocumentId, F.PassageId, DEM.DocumentEntityMentionId, DEM.DocumentEntityId, "
				<< " D.DocumentSourceTypeId, D.SourceLanguage, D.DocumentPublicationDate, F.Confidence, F.FactTypeId, DC.StringValue, F.extractorid, D.epochid FROM "
				<< "Document D, DocumentChunk DC, MentionFactArgument MFA, DocumentEntityMention DEM, EntityActorMap EAM, Fact F WHERE "
			    << " MFA.DocumentEntityMentionId = DEM.DocumentEntityMentionId "
				<< " AND MFA.FactId = F.FactId "
				<< " AND DEM.DocumentEntityId = EAM.DocumentEntityId "
				<< " AND F.SupportChunkId = DC.DocumentChunkId "
				<< " AND F.DocumentId = D.DocumentId "
				<< " AND EAM.ActorMatchConfidence > " << MIN_ACTOR_MATCH_CONFIDENCE // we only want good names here, of course!
				<< " AND F.FactTypeId IN " << DatabaseConnection::makeList(fact_type_ids)
				<< " AND EAM.ActorId IN " << DatabaseConnection::makeList(actor_ids)
				<< " AND F.deleted='f'";

	std::map<int, PGFact_ptr> factMap;
	std::list<int> factIds;
	std::map<int, std::list<int> > factIdsByFactType;
	std::map<int, std::list<PGFact_ptr> > factsByFactType;

	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		PGFact::pg_fact_info_t fact_info;
		fact_info.fact_id = row.getCellAsInt32(0);
		fact_info.document_id = row.getCellAsInt32(1);
		fact_info.passage_id = row.getCellAsInt32(2);
		fact_info.mention_id = row.getCellAsInt32(3);
		fact_info.entity_id = row.getCellAsInt32(4);	
		fact_info.language = LanguageAttribute::getFromString(row.getCellAsWString(6).c_str());
		std::string rawdate = row.getCellAsString(7);
		if (rawdate != "")
			fact_info.document_date = boost::gregorian::from_string(rawdate);
		fact_info.score = row.getCellAsDouble(8);
		fact_info.fact_type_id = row.getCellAsInt32(9);
		fact_info.support_chunk_value = row.getCellAsWString(10);
		fact_info.db_fact_type = ProfileSlot::DOCUMENT;
		fact_info.score_group = -1;
		fact_info.source_type = PGFact::TEXT;
		fact_info.status = PGFact::VALID;
		fact_info.source_id = row.getCellAsInt32(11);
		fact_info.epoch_id = row.getCellAsInt32(12);
		fact_info.is_serif = (_serif_extractor_id == fact_info.source_id);
			
		std::string source_type = convertIdToString("DocumentSourceType", row.getCellAsInt32(5));
		if (source_type == "ocr")
			fact_info.source_type = PGFact::OCR;
		else if (source_type == "asr" || source_type == "stt")
			fact_info.source_type = PGFact::STT;

		PGFact_ptr fact = boost::make_shared<PGFact>(fact_info);
		factMap[fact_info.fact_id] = fact;
		results.push_back(factMap[fact_info.fact_id]);
		factIds.push_back(fact_info.fact_id);
		factIdsByFactType[fact_info.fact_type_id].push_back(fact_info.fact_id);
		factsByFactType[fact_info.fact_type_id].push_back(fact);
	}

	if (factIds.size() == 0)
		return results;

	// Do we have a fact category with too many facts?
	bool size_problem = false;
	typedef std::pair<int, std::list<int> > my_pair_t;		
	if (_max_facts_per_fact_type != 0) {
		BOOST_FOREACH(my_pair_t mypair, factIdsByFactType) {
			if (mypair.second.size() > _max_facts_per_fact_type)
				size_problem = true;
		}
	}		

	size_t original_fact_count = factIds.size();
	if (size_problem) {
		factIds.clear();
		// Go through each fact type
		// If it doesn't hit the maximum, just add those fact IDs to the list and move on
		// If it does, sort the facts for that fact type by date/ID and then walk
		//   backwards through the list until we hit the max. Print a warning.
		for (std::map<int, std::list<int> >::iterator map_iter = factIdsByFactType.begin(); map_iter != factIdsByFactType.end(); map_iter++) {
			my_pair_t mypair = *map_iter;
			if (mypair.second.size() <= _max_facts_per_fact_type) {
				factIds.splice(factIds.begin(), mypair.second);
				continue;
			}
			std::list<PGFact_ptr> facts = factsByFactType[mypair.first];
			facts.sort(PGFact::compareByDate);
			size_t count = 0;
			boost::gregorian::date firstDate = (*(facts.begin()))->getDocumentDate();
			boost::gregorian::date cutoffDate;
			boost::gregorian::date lastDate = (*(facts.rbegin()))->getDocumentDate();
			for (std::list<PGFact_ptr>::reverse_iterator iter = facts.rbegin(); iter != facts.rend(); iter++) {
				factIds.push_back( (*iter)->getFactId() );
				count++;
				if (count == _max_facts_per_fact_type) {
					cutoffDate = (*iter)->getDocumentDate();
					break;
				}
			}
			std::stringstream msg;
			msg << "Pruned fact list from " << facts.size() << " to " << _max_facts_per_fact_type << " for fact type ";
			msg << convertIdToString("FactType", mypair.first);
			msg << ". Original fact date range was " << boost::gregorian::to_simple_string(firstDate) << " to ";
			msg << boost::gregorian::to_simple_string(lastDate) << ", ";
			msg << "earliest date is now " << boost::gregorian::to_simple_string(cutoffDate);
			SessionLogger::warn("PG") << msg.str() << std::endl;
		}
	}	
	size_t new_fact_count = factIds.size();
	if (original_fact_count != new_fact_count) {
		SessionLogger::warn("PG") << "Pruned facts from " << original_fact_count << " to " << new_fact_count << std::endl;
	}

	std::string nullString = "";

	// Add mention arguments
	std::stringstream mentionArgStream;
	mentionArgStream << "SELECT MFA.DocumentEntityMentionId, EAM.ActorId, DE.EntityType, MFA.RoleId, DEM.Confidence, DC.StringValue, A.CanonicalName, MFA.FactId "
		<< " FROM MentionFactArgument MFA, DocumentEntityMention DEM, DocumentEntity DE, EntityActorMap EAM, DocumentChunk DC, Actor A WHERE "
		<< " MFA.DocumentEntityMentionId = DEM.DocumentEntityMentionId "
		<< " AND DEM.DocumentChunkId = DC.DocumentChunkId"
		<< " AND DEM.DocumentEntityId = EAM.DocumentEntityId "
		<< " AND DEM.DocumentEntityId = DE.DocumentEntityId "
		<< " AND EAM.ActorId = A.ActorId "
		<< " AND EAM.ActorMatchConfidence > " << MIN_ACTOR_MATCH_CONFIDENCE // we only want good names here, of course!				
		<< " AND MFA.FactId IN " << DatabaseConnection::makeList(factIds);

	// Keep track of the mentions (and the args they land in) so we can get their SERIF confidences in the next step
	std::map<int, std::vector<PGFactArgument_ptr> > mentionToArgMap;
	std::set<int> mentionIds;
	for (DatabaseConnection::RowIterator row = _db->iter(mentionArgStream.str().c_str()); row!=_db->end(); ++row) {
		int ment_id = row.getCellAsInt32(0);
		int actor_id = row.getCellAsInt32(1);
		std::string entity_type = row.getCellAsString(2);
		std::string role_name = convertIdToString("Role", row.getCellAsInt32(3));
		double confidence = row.getCellAsDouble(4);
		std::wstring literal_string = row.getCellAsWString(5); // literal text from document
		std::wstring resolved_string = row.getCellAsWString(6); // Actor canonical name
		int fact_id = row.getCellAsInt32(7);
		PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(ment_id, actor_id, entity_type, literal_string, resolved_string, 
			role_name, confidence, (ment_id == factMap[fact_id]->getMentionId()));
		factMap[fact_id]->addArgument(arg);
		mentionToArgMap[ment_id].push_back(arg);
		mentionIds.insert(ment_id);
	}
	
	// Set SERIF mention confidences; this is SOLELY to be backwards-compatible at this point
	// It's basically OK that it's a no-op in the modern world, since the SerifDocumentEntityMentionConfidence 
	//    will be empty, which means it's fast.
	// Note that we could not do this in the larger query above, since not all mentions may have 
	//    a SERIF mention confidence (in fact, in the modern world, none do)
	std::stringstream serifConfidenceArgStream;
	serifConfidenceArgStream << "SELECT DocumentEntityMentionId, SerifMentionConfidenceTypeId FROM SerifDocumentEntityMentionConfidence WHERE DocumentEntityMentionId IN "
							 <<  DatabaseConnection::makeList(mentionIds);
	for (DatabaseConnection::RowIterator row = _db->iter(serifConfidenceArgStream.str().c_str()); row!=_db->end(); ++row) {
		int ment_id = row.getCellAsInt32(0);
		std::string conf_name = convertIdToString("SerifMentionConfidenceType", row.getCellAsInt32(1));
		MentionConfidenceAttribute conf;
		try {				
			conf = MentionConfidenceAttribute::getFromString(UnicodeUtil::toUTF16StdString(conf_name).c_str());
		} catch (...) {
			SessionLogger::err("PG") << "Can't resolve the SERIF Mention Confidence attribute " << conf_name.c_str() << std::endl;
			continue;
		}
		BOOST_FOREACH(PGFactArgument_ptr arg, mentionToArgMap[ment_id]) {
			arg->setSERIFMentionConfidence(conf);
		}
	}

	// Add time arguments
	std::stringstream timeArgStream;
	timeArgStream << "SELECT TMFA.DocumentTimeMentionId, DC.StringValue, DTM.ResolvedTimex, TMFA.RoleId, TMFA.FactId"
		<< " FROM TimeMentionFactArgument TMFA, DocumentTimeMention DTM, DocumentChunk DC WHERE "
		<< " TMFA.DocumentTimeMentionId = DTM.DocumentTimeMentionId"
		<< " AND DTM.DocumentChunkId = DC.DocumentChunkId"
		<< " AND TMFA.FactId IN " << DatabaseConnection::makeList(factIds);

	for (DatabaseConnection::RowIterator row = _db->iter(timeArgStream.str().c_str()); row!=_db->end(); ++row) {	
		int ment_id = row.getCellAsInt32(0);
		std::wstring literal_string = row.getCellAsWString(1); // literal text from document
		std::wstring resolved_string = row.getCellAsWString(2); // timex
		std::string role_name = convertIdToString("Role", row.getCellAsInt32(3));
		int fact_id = row.getCellAsInt32(4);
		double confidence = 1.0;
		PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(ment_id, -1, nullString, literal_string, resolved_string, role_name, confidence, false);
		factMap[fact_id]->addArgument(arg);
	}

	// Add chunk arguments WITH resolutions
	std::stringstream chunkArgStream;
	chunkArgStream << "SELECT CFA.ChunkFactArgumentId, CFA.DocumentChunkId, DC.StringValue, DCR.ResolvedStringValue, CFA.RoleId, CFA.FactId FROM ChunkFactArgument CFA, DocumentChunk DC, DocumentChunkResolution DCR WHERE "
		<< " CFA.DocumentChunkId  = DC.DocumentChunkId "
		<< " AND DC.DocumentChunkId = DCR.DocumentChunkId "
		<< " AND CFA.FactId IN " << DatabaseConnection::makeList(factIds);

	std::set<int> chunkArgumentsAdded;
	for (DatabaseConnection::RowIterator row = _db->iter(chunkArgStream.str().c_str()); row!=_db->end(); ++row) {		
		int cfa_id = row.getCellAsInt32(0);
		int chunk_id = row.getCellAsInt32(1);
		std::wstring literal_string = row.getCellAsWString(2); // literal text from document
		std::wstring resolved_string = row.getCellAsWString(3); // resolved text from document
		std::string role_name = convertIdToString("Role", row.getCellAsInt32(4));
		int fact_id = row.getCellAsInt32(5);
		PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(chunk_id, -1, nullString, literal_string, resolved_string, role_name, 1.0, false);
		factMap[fact_id]->addArgument(arg);
		chunkArgumentsAdded.insert(cfa_id);
	}

	// Add chunk arguments WITHOUT resolutions-- just use the literal string as the resolved string for now
	std::stringstream chunkArgStream2;
	chunkArgStream2 << "SELECT CFA.ChunkFactArgumentId, CFA.DocumentChunkId, DC.StringValue, CFA.RoleId, CFA.FactId FROM ChunkFactArgument CFA, DocumentChunk DC WHERE "
		<< " CFA.DocumentChunkId  = DC.DocumentChunkId "
		<< " AND CFA.FactId IN " << DatabaseConnection::makeList(factIds);

	for (DatabaseConnection::RowIterator row = _db->iter(chunkArgStream2.str().c_str()); row!=_db->end(); ++row) {					
		int cfa_id = row.getCellAsInt32(0);
		if (chunkArgumentsAdded.find(cfa_id) != chunkArgumentsAdded.end())
			continue;
		int chunk_id = row.getCellAsInt32(1);
		std::wstring literal_string = row.getCellAsWString(2); // literal text from document
		std::wstring resolved_string = literal_string;
		boost::replace_all(resolved_string,L"\n",L" ");
		std::string role_name = convertIdToString("Role", row.getCellAsInt32(3));
		int fact_id = row.getCellAsInt32(4);
		PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(chunk_id, -1, nullString, literal_string, resolved_string, role_name, 1.0, false);
		factMap[fact_id]->addArgument(arg);
	}

	// Add dates		
	std::stringstream dateArgStream;
	dateArgStream << "SELECT FD.FactDateId, FD.DateTypeId, DC.StringValue, DTM.ResolvedTimex, FD.FactId FROM FactDate FD, DocumentTimeMention DTM, DocumentChunk DC WHERE "
		<< " FD.DocumentTimeMentionId = DTM.DocumentTimeMentionId "
		<< " AND DTM.DocumentChunkId = DC.DocumentChunkId"
		<< " AND FD.FactId IN " << DatabaseConnection::makeList(factIds);

	for (DatabaseConnection::RowIterator row = _db->iter(dateArgStream.str().c_str()); row!=_db->end(); ++row) {				
		int date_id = row.getCellAsInt32(0);			
		std::string date_type = convertIdToString("DateType", row.getCellAsInt32(1));
		std::wstring literal_string = row.getCellAsWString(2); // literal text from document
		std::wstring resolved_string = row.getCellAsWString(3); // timex
		int fact_id = row.getCellAsInt32(4);
		DatabaseFactDate_ptr date = boost::make_shared<DatabaseFactDate>(date_id, factMap[fact_id], date_type, literal_string, resolved_string);
		factMap[fact_id]->addDate(date);
	}

	return results;
}

// 
// Convert fact type names to fact type IDs
//
std::vector<int> PGDatabaseManager::getFactTypeIds(std::vector<std::string>& fact_type_names) {
	// There might be none of these, FYI, in which case we won't constrain on fact type
	std::vector<int> fact_type_ids;
	BOOST_FOREACH(std::string fact_type_name, fact_type_names) {
		int fact_type_id = convertStringToId("FactType", fact_type_name, false);
		if (fact_type_id == -1) {
			SessionLogger::dbg("PG") << "failed to find a fact type named '" << fact_type_name.c_str() << "' in the _db" << std::endl;
			continue;
		}	
		fact_type_ids.push_back(fact_type_id);
	}
	return fact_type_ids;
}

//
// Returns a list of Fact objects represenging the external facts for these actors
//
std::vector<PGFact_ptr> PGDatabaseManager::getExternalFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names) {
	std::vector<int> fact_type_ids = getFactTypeIds(fact_type_names);
	return getKBOrExternalFacts(actor_ids, fact_type_ids, "External", false);
}
std::vector<PGFact_ptr> PGDatabaseManager::getExternalFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids) {
	return getKBOrExternalFacts(actor_ids, fact_type_ids, "External", false);
}

//
// Returns a list of Fact objects represenging the KB facts for these actors
//
std::vector<PGFact_ptr> PGDatabaseManager::getKBFacts(std::vector<int>& actor_ids, std::vector<std::string>& fact_type_names, bool ignore_status) {
	std::vector<int> fact_type_ids = getFactTypeIds(fact_type_names);
	return getKBOrExternalFacts(actor_ids, fact_type_ids, "KB", ignore_status);
}
std::vector<PGFact_ptr> PGDatabaseManager::getKBFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids, bool ignore_status) {
	return getKBOrExternalFacts(actor_ids, fact_type_ids, "KB", ignore_status);
}

//
// Returns a list of Fact objects represenging the KBFacts or ExternalFacts for these actors
//
std::vector<PGFact_ptr> PGDatabaseManager::getKBOrExternalFacts(std::vector<int>& actor_ids, std::vector<int>& fact_type_ids, std::string prefix, bool ignore_status) {
	
	std::vector<PGFact_ptr> results;	
	if (actor_ids.size() == 0)
		return results;
	
	std::stringstream queryStream;

	queryStream << "SELECT F." << prefix << "FactId, F." << prefix << "FactTypeId, F.Confidence, FA." << prefix << "FactArgumentId,";
	queryStream << " FA.ActorId, FA.RoleId, FA.StringValue ";
	if (prefix == "KB")
		queryStream << ", F.KBFactStatusId, F.LastUpdateEpoch ";
	else
		queryStream << ", F.SourceId "; // for external facts only
	queryStream << "FROM " << prefix << "Fact F, " << prefix << "FactArgument FA ";
	queryStream << " WHERE F." << prefix << "FactId = FA." << prefix << "FactId";
	queryStream << " AND FA.ActorId IN " << DatabaseConnection::makeList(actor_ids);
	if (fact_type_ids.size() != 0)
		queryStream << " AND F." << prefix << "FactTypeId IN " << DatabaseConnection::makeList(fact_type_ids);

	boost::gregorian::date nullDate;
	std::string nullString = "";
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		PGFact::pg_fact_info_t fact_info;
		fact_info.fact_id = row.getCellAsInt32(0);
		fact_info.fact_type_id = row.getCellAsInt32(1);
		fact_info.score = row.getCellAsDouble(2);		
		fact_info.score_group = -1;
		fact_info.document_id = -1;
		fact_info.passage_id = -1;
		fact_info.mention_id = -1;
		fact_info.entity_id = -1;
		fact_info.language = Language::UNSPECIFIED;
		fact_info.support_chunk_value = L"";
		fact_info.db_fact_type = ProfileSlot::EXTERNAL;
		fact_info.source_type = PGFact::NOT_APPLICABLE;			
		fact_info.source_id = -1;		
		fact_info.is_serif = false;
		fact_info.status = PGFact::VALID;
		if (prefix == "KB") {
			int statusid = row.getCellAsInt32(7);
			if (_factStatusById.find(statusid) != _factStatusById.end())
				fact_info.status = _factStatusById[statusid];
			else {
				SessionLogger::err("PG") << "Unknown KB fact status id '" << statusid << "'; skipping fact" << std::endl;
				continue;
			}
			fact_info.epoch_id = row.getCellAsInt32(8);
		} else {
			fact_info.source_id = row.getCellAsInt32(7);
			fact_info.epoch_id = -1;
		}
		if (!ignore_status) {
			if (fact_info.status == PGFact::USER_DELETED || fact_info.status == PGFact::UNTRUSTED)
				continue;
		}		

		PGFact_ptr fact = boost::make_shared<PGFact>(fact_info);

		int arg_id = row.getCellAsInt32(3);
		int actor_id = row.getCellAsInt32(4);
		int role_id = row.getCellAsInt32(5);
		std::wstring string_value = row.getCellAsWString(6);

		// Use the confidence from the fact for each argument... not ideal but what else to do?
		// For DBPedia these are currently hard-coded to .95 in the import process, by the way
		PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(arg_id, actor_id, nullString, string_value, string_value, convertIdToString("Role", role_id), fact->getScore(), true);
		fact->addArgument(arg);

		results.push_back(fact);
	}

	BOOST_FOREACH(PGFact_ptr fact, results) {
		
		// Add additional arguments
		std::stringstream argStream;
		argStream << "SELECT " << prefix << "FactArgumentId, ActorId, RoleId, StringValue FROM " << prefix << "FactArgument WHERE " << prefix << "FactId = " << fact->getFactId();

		for (DatabaseConnection::RowIterator row = _db->iter(argStream.str().c_str()); row!=_db->end(); ++row) {
			int arg_id = row.getCellAsInt32(0);
			bool already_added = false;
			BOOST_FOREACH(PGFactArgument_ptr arg, fact->getAllArguments()) {
				if (arg->getPrimaryId() == arg_id)
					already_added = true;
			}
			if (already_added)
				continue;
			int actor_id = row.getCellAsInt32(1);
			int role_id = row.getCellAsInt32(2);
			std::wstring string_value = row.getCellAsWString(3);
			// Use the confidence from the fact for each argument... not ideal but what else to do?
			// For DBPedia these are currently hard-coded to .95 in the import process, by the way
			PGFactArgument_ptr arg = boost::make_shared<PGFactArgument>(arg_id, actor_id, nullString, string_value, string_value, convertIdToString("Role", role_id), fact->getScore(), false);
			fact->addArgument(arg);
		}
	}	

	return results;
}

//
// Get ProfileFacts that involve a particular actor
//
std::map<int, int> PGDatabaseManager::getProfileFacts(std::vector<int>& actor_ids) {	
	std::map<int, int> results;	
	if (actor_ids.size() == 0)
		return results;
	
	std::stringstream queryStream;
	queryStream << "SELECT KbFactId, ProfileFactId from ProfileFact WHERE ActorId IN " << DatabaseConnection::makeList(actor_ids);

	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		results[row.getCellAsInt32(0)] = row.getCellAsInt32(1);
	}
	return results;
}

//
// Get BinaryRelations that involve a particular actor
//
std::map<int, int> PGDatabaseManager::getBinaryRelations(std::vector<int>& actor_ids) {	
	std::map<int, int> results;	
	if (actor_ids.size() == 0)
		return results;
	
	std::stringstream queryStream;
	queryStream << "SELECT KbFactId, BinaryRelationId from BinaryRelation ";
	queryStream << "WHERE ActorId1 IN " << DatabaseConnection::makeList(actor_ids) << " OR ActorId2 IN " << DatabaseConnection::makeList(actor_ids);

	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		results[row.getCellAsInt32(0)] = row.getCellAsInt32(1);
	}
	return results;
}

//
// Get list of employee actor Ids for a given employer
//
std::set<int> PGDatabaseManager::getReliableEmployees(int employer_id) {

	std::vector<int> actor_ids;
	actor_ids.push_back(employer_id);

	std::vector<std::string> factTypes;
	factTypes.push_back(ProfileSlot::KB_EMPLOYMENT_FACT_TYPE);
	std::vector<PGFact_ptr> facts = getKBFacts(actor_ids, factTypes, false); // don't include deleted/untrusted facts
	std::set<int> employee_ids;
	BOOST_FOREACH(PGFact_ptr fact, facts) {
		if (fact->getScore() == 0)
			continue;
		std::vector<PGFactArgument_ptr> args = fact->getAllArguments();
		std::set<int> temp_employee_ids;
		bool is_match = false;
		for (std::vector<PGFactArgument_ptr>::iterator iter = args.begin(); iter != args.end(); iter++) {
			if ((*iter)->getRole() == ProfileSlot::KB_EMPLOYEE_ROLE)
				temp_employee_ids.insert((*iter)->getActorId());
			if ((*iter)->getRole() == ProfileSlot::KB_EMPLOYER_ROLE && (*iter)->getActorId() == employer_id)
				is_match = true;
		}
		if (is_match) {
			for (std::set<int>::iterator iter = temp_employee_ids.begin(); iter != temp_employee_ids.end(); iter++) {
				employee_ids.insert(*iter);
			}
		}
	}

	return employee_ids;
}

//
// Returns list of actor IDs which appear in documents for this epoch. 
// (Used to determine which profiles should be updated for an epoch.)
// Can be consrained by entity type or minimum appearance count
//
std::set<int> PGDatabaseManager::getActorsInEpoch(int epoch_id, int min_count, std::string entity_type) {

	std::stringstream queryStream;
 
	queryStream << "SELECT A.actor_id, count(A.actor_id) FROM Document D, DocumentChunk DC, DocumentEntityMention DEM, EntityActorMap EAM, Actor A WHERE "
				<< " DEM.DocumentEntityId = EAM.DocumentEntityId "
				<< " AND DEM.DocumentChunkId = DC.DocumentChunkId "
				<< " AND DC.DocumentId = D.DocumentId "
				<< " AND EAM.ActorId = A.ActorId "
				<< " AND D.EpochId = " << epoch_id;
	if (entity_type != "NONE" && entity_type != "UNKNOWN")
		queryStream << " AND A.EntityType = '" << DatabaseConnection::sanitize(entity_type) << "'"; 
				
	std::set<int> actorIds;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		int count = row.getCellAsInt32(1);
		if (count >= min_count)
			actorIds.insert(row.getCellAsInt32(0));
	}

	return actorIds;
}

//
// Get all actors that have been requested by a user.
//
std::set<int> PGDatabaseManager::getRequestedActors(){

	std::stringstream queryStream;
	queryStream << "SELECT ActorId from UserRequestedActors";

	std::set<int> actors;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		actors.insert(row.getCellAsInt32(0));
	}

	return actors;
}

//
// Converts type strings to database IDs
//
int PGDatabaseManager::convertStringToId(std::string table_name, std::string str, bool allow_new) {

	std::pair<std::string, std::string> key = std::make_pair(table_name, str);

	// First check if we've already cached this
	std::map<std::pair<std::string, std::string>, int>::const_iterator iter = _stringToId.find(key);
	if (iter != _stringToId.end()) {
		return iter->second;
	}

	// Otherwise, query the database and cache the result
	std::stringstream queryStream;
	queryStream << "SELECT " << table_name << "Id FROM " << table_name << " WHERE " << table_name << "Name = '" << DatabaseConnection::sanitize(str) << "'"; 
	int id = -1;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		id = row.getCellAsInt32(0);
	}

	if (id == -1 && allow_new) {
		std::stringstream insertStream;
		insertStream << "INSERT INTO " << table_name << " (" << table_name << "Name) VALUES ('" << DatabaseConnection::sanitize(str) << "')"; 
		try {
			id = insertReturningInt(insertStream.str(), table_name + "Id");
		} catch (...) {
			// It's possible that due to multiple parallel processes, the item might have been populated in between these two queries.
			// So, let's try the first query again.
			for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
				id = row.getCellAsInt32(0);
			}
		}
	}
	
	if (id == -1) {
		std::stringstream err;
		err << "convertStringToId didn't find (or couldn't insert) the id " << id << std::endl;
		SessionLogger::dbg("PG") << err.str();
	}

	// Cache this result
	_stringToId[key] = id;

	return id;
}


//
// Converts database IDs to type strings
//
std::string PGDatabaseManager::convertIdToString(std::string table_name, int id) {

	std::pair<std::string, int> key = std::make_pair(table_name, id);

	// First check if we've already cached this
	std::map<std::pair<std::string, int>, std::string>::const_iterator iter = _idToString.find(key);
	if (iter != _idToString.end()) {
		return iter->second;
	}

	// Otherwise, query the database and cache the result
	std::stringstream queryStream;
	queryStream << "SELECT " << table_name << "Name FROM " << table_name << " WHERE " << table_name << "Id = " << id; 

	std::string name = "";
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		name = row.getCellAsString(0);
	}

	if (name.size() == 0) {
		std::stringstream err;
		err << "convertIdToString didn't find the id " << id<< std::endl;
		SessionLogger::dbg("PG") << err.str();
		throw PGDatabaseManagerException(err.str().c_str());
	}

	// Cache this result
	_idToString[key] = name;

	return name;
}

int PGDatabaseManager::getDisplaySlotId(ProfileSlot_ptr slot) {
	std::pair<std::string, std::string> key = std::make_pair("ProfileDisplaySlot", slot->getDisplayName());
	
	// First check if we've already cached this
	std::map<std::pair<std::string, std::string>, int>::const_iterator iter = _stringToId.find(key);
	if (iter != _stringToId.end()) {
		return iter->second;
	}

	// Otherwise, query the database and cache the result
	std::stringstream queryStream;
	queryStream << "SELECT ProfileDisplaySlotId FROM ProfileDisplaySlot WHERE DisplaySlotName = '" << DatabaseConnection::sanitize(slot->getDisplayName()) << "'";
	
	int id = -1;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		id = row.getCellAsInt32(0);
	}

	if (id == -1) {
		std::stringstream insertStream;
		insertStream << "INSERT INTO ProfileDisplaySlot (DisplaySlotName, DisplaySlotRank, DisplaySlotType, SortOrder, IsInfobox) VALUES("
				<< "'" << DatabaseConnection::sanitize(slot->getDisplayName()) << "', "
				<< slot->getRank() << ", "
				<< "'" << DatabaseConnection::sanitize(slot->getDisplayType()) << "', "
				<< "'" << slot->getSortOrder() << "', ";			
		if (slot->isInfoboxSlot())
			insertStream << "TRUE)";
		else insertStream << "FALSE)";
		id = insertReturningInt(insertStream.str(), "ProfileDisplaySlotId");
	}

	// Cache this result
	_stringToId[key] = id;

	return id;
}

//
// Gets possible actor IDs for a given entity name string.
//
std::vector<PGActorInfo_ptr> PGDatabaseManager::getPossibleActorsForName(std::wstring actor_name) {

	// TODO_AZ: Alex Zamanian should make this match what happens in the actor-match stage better!
	// This is probably horribly wrong and you can't just do exact match against name string fields.
	// Right now this also does not deal with corpus count, which might be important

	std::vector<PGActorInfo_ptr> results;

	boost::to_lower(actor_name);

	std::wstringstream queryStream;
	queryStream << L"SELECT A.ActorId, A.CanonicalName, A.EntityType, AST.Confidence FROM ActorString AST, Actor A "
				<< L"WHERE AST.ActorId = A.ActorId AND lower(String) = '" <<  DatabaseConnection::sanitize(actor_name) << L"'";
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		int actor_id = row.getCellAsInt32(0);
		std::wstring canonical_actor_name = row.getCellAsWString(1);
		std::string entity_type = row.getCellAsString(2);
		double confidence = row.getCellAsDouble(3);
		PGActorInfo_ptr am = boost::make_shared<PGActorInfo>(actor_id, canonical_actor_name, entity_type, confidence);
		results.push_back(am);
	}

	return results;
}

std::vector<std::wstring> PGDatabaseManager::getDocumentCanonicalNamesForActor(int actor_id) {
	std::wstringstream queryStream;
	queryStream << "SELECT DE.CanonicalName from DocumentEntity DE, EntityActorMap EAM "
				<< " WHERE DE.DocumentEntityId = EAM.DocumentEntityId AND EAM.ActorId = " << actor_id
				<< " AND EAM.ActorMatchConfidence > " << MIN_ACTOR_MATCH_CONFIDENCE
				<< " GROUP BY DE.CanonicalName";

	std::vector<std::wstring> results;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		results.push_back(row.getCellAsWString(0));
	}
	return results;
}


//
// Gets the "best" possible actor ID for a given entity name string (and optionally profile type)
//
PGActorInfo_ptr PGDatabaseManager::getBestActorForName(std::wstring actor_name,  Profile::profile_type_t profileType) {

	std::vector<PGActorInfo_ptr> actorMatches = getPossibleActorsForName(actor_name);

	PGActorInfo_ptr best = PGActorInfo_ptr();
	BOOST_FOREACH(PGActorInfo_ptr ami, actorMatches) {
		if (profileType != Profile::UNKNOWN) {
			if (Profile::getStringForProfileType(profileType) != ami->getEntityType())
				continue;
		}
		if (best == PGActorInfo_ptr() || ami->getConfidence() > best->getConfidence()) {
			best = ami;
		}
	}

	return best;
}

//
// Fill in a PGActorInfo struct for this actor id
//
PGActorInfo_ptr PGDatabaseManager::getActorInfoForId(int actor_id) {

	std::vector<PGActorInfo_ptr> results;

	std::stringstream queryStream;
	queryStream << "SELECT ActorId, CanonicalName, EntityType FROM Actor WHERE ActorId = " << actor_id;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {
		int actor_id = row.getCellAsInt32(0);
		std::wstring canonical_actor_name = row.getCellAsWString(1);
		std::string entity_type = row.getCellAsString(2);
		double confidence = 0;
		PGActorInfo_ptr am = boost::make_shared<PGActorInfo>(actor_id, canonical_actor_name, entity_type, confidence);
		// Not sure we're allowed to break out of these; I had trouble with that before :(
		results.push_back(am);
	}

	if (results.size() == 0) {
		return PGActorInfo_ptr();
	} else return results.at(0);
}

// 
//
// UPLOAD FUNCTIONS
//
//

//
// Upload a profile!
//
bool PGDatabaseManager::uploadProfile(Profile_ptr profile, ProfileConfidence_ptr confidenceModel, bool print_to_screen) {

	profile->prepareForUpload(confidenceModel, print_to_screen);

	// For debugging purposes, sometimes we want to call the 'prepare' function but dont't want to actually modify the DB
	if (_skip_database_upload)
		return true;

	std::vector<int> actor_ids;
	actor_ids.push_back(profile->getActorId());
	
	// Get all the existing stuff for this actor at once, to minimize database calls
	// But, be sure only to get fact types that are targets for this type of actor, or you'll
	//   risk getting stuff you aren't trying to generate for this actor, which could result in
	//   other facts being marked as untrusted. This happens, for example, with Nationality facts.
	//   Countries don't generate them, but they are involved in them.

	std::vector<int> targetKBFactTypeIds;
	for (ProfileSlotMap::const_iterator iter = profile->getSlots().begin(); iter != profile->getSlots().end(); ++iter) {		
		targetKBFactTypeIds.push_back((*iter).second->getKBFactTypeId());
	}
	
	std::vector<PGFact_ptr> existingKBFacts = getKBFacts(actor_ids, targetKBFactTypeIds, true);

	// NOTE: These may still include profile facts and binary relations NOT targeted by this type of actor;
	//  this is dealt with in the cleanup functions
	std::map<int, int> existingProfileFacts = getProfileFacts(actor_ids);
	std::map<int, int> existingBinaryRelations = getBinaryRelations(actor_ids);

	ProfileSlotMap& slotMap = profile->getSlots();
	std::set<int> usedKBFactIds;
	for (ProfileSlotMap::iterator iter = slotMap.begin(); iter != slotMap.end(); iter++) {
		uploadProfileSlot(profile, iter->second, usedKBFactIds, existingKBFacts, existingProfileFacts, existingBinaryRelations);
	}
	cleanUpProfileInformation(usedKBFactIds, existingKBFacts, existingProfileFacts, existingBinaryRelations);

	markActorProfileUpdated(profile->getActorId());

	return true;
}

//
// Upload a profile slot!
//
void PGDatabaseManager::uploadProfileSlot(Profile_ptr profile, ProfileSlot_ptr slot, std::set<int>& usedKBFactIds, std::vector<PGFact_ptr>& existingKBFacts,
										  std::map<int, int>& existingProfileFacts, std::map<int, int>& existingBinaryRelations) {

	// Assume that output is up-to-date and ready to go here; don't regenerate

	// Upload hypotheses and store the KB facts we upload, so we can later clean up those which are orphaned
	//  (we want to do this for the whole profile at once, since there may be overlap)
	BOOST_FOREACH(GenericHypothesis_ptr hypoth, slot->getOutputHypotheses()) {
		int kb_fact_id = uploadHypothesis(profile, slot, hypoth, existingKBFacts, existingProfileFacts, existingBinaryRelations);
		if (kb_fact_id != -1) 
			usedKBFactIds.insert(kb_fact_id);
	}
}

void PGDatabaseManager::cleanUpProfileInformation(std::set<int>& usedKBFacts, std::vector<PGFact_ptr>& existingKBFacts,
										  std::map<int, int>& existingProfileFacts, std::map<int, int>& existingBinaryRelations) 
{
	// Identify KB facts to prune; they must be both a target (in existingKBFacts) and not used
	std::set<int> factsToPrune;
	BOOST_FOREACH(PGFact_ptr kbFact, existingKBFacts) {
		if (kbFact->getEpochId() == _epoch_id) {
			// This was already considered to be true this epoch, so don't mess with it!
			continue;
		}
		if (usedKBFacts.find(kbFact->getFactId()) == usedKBFacts.end()) {			
			factsToPrune.insert(kbFact->getFactId());
		}
	}

	// Remove ProfileFact and BinaryRelation rows pointed to by orphaned KB facts
	cleanUpCacheTable(factsToPrune, existingProfileFacts, "ProfileFact");
	cleanUpCacheTable(factsToPrune, existingBinaryRelations, "BinaryRelation");

	// Set confidences of orphaned KB facts to zero for now
	BOOST_FOREACH(PGFact_ptr kbFact, existingKBFacts) {
		if (usedKBFacts.find(kbFact->getFactId()) == usedKBFacts.end()) {
			if (kbFact->getStatus() == PGFact::VALID)
				updateExistingKBFact(kbFact->getFactId(), 0.0, _untrusted_fact_status_id);	
			else updateExistingKBFact(kbFact->getFactId(), 0.0);	
		}
	}
}

// 
// Mark as untrustworthy and untouched profile facts or binary relations
// Note that we only do this if it was previously marked as valid; if
//   it was previously marked as user-deleted or trimmed, we leave it alone
//
void PGDatabaseManager::cleanUpCacheTable(std::set<int>& factsToPrune, std::map<int, int>& existingCacheFacts, std::string table_name) 
{
	for (std::map<int,int>::iterator iter = existingCacheFacts.begin(); iter != existingCacheFacts.end(); iter++) {
		int kb_fact_id = (*iter).first;
		if (factsToPrune.find(kb_fact_id) != factsToPrune.end()) {
			std::stringstream queryStream;
			queryStream << "UPDATE " << table_name << " SET KBFactStatusId = " << _untrusted_fact_status_id
					    << " WHERE " << table_name << "Id = " << (*iter).second
						<< " AND KBFactStatusId = " << _valid_fact_status_id;
			_db->exec(queryStream.str());		
		}
	}
}

//
// Upload a profile slot hypothesis (i.e. a UI fact)!
//
int PGDatabaseManager::uploadHypothesis(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, std::vector<PGFact_ptr>& existingKBFacts,
										  std::map<int, int>& existingProfileFacts, std::map<int, int>& existingBinaryRelations) {

	// First, see if this hypothesis already exists as a KbFact
	PGFact_ptr kbFact = findMatchingKBFact(profile, slot, hypoth, existingKBFacts);
	int kb_fact_id = -1;

	if (kbFact != PGFact_ptr()) {
		
		kb_fact_id = kbFact->getFactId();
		if (kbFact->getStatus() == PGFact::USER_DELETED)
			return kb_fact_id;
		updateExistingKBFact(kb_fact_id, hypoth->getConfidence(), _valid_fact_status_id, hypoth);
		
		std::map<int,int>::iterator iter = existingProfileFacts.find(kb_fact_id);
		if (iter == existingProfileFacts.end()) {
			createProfileFact(profile, slot, hypoth, kb_fact_id);
		} else {
			markCacheEntryValid( "ProfileFact", (*iter).second );
		}
		
		if (slot->isBinaryRelation()) {
			std::map<int,int>::iterator iter = existingBinaryRelations.find(kb_fact_id);
			if (iter == existingBinaryRelations.end()) {
				createBinaryRelation(profile, slot, hypoth, kb_fact_id);
			} else {
				markCacheEntryValid( "BinaryRelation", (*iter).second );
			}
		}

	} else {

		// Create the KB fact for this hypothesis
		kb_fact_id = createKBFact(profile, slot, hypoth);
		createProfileFact(profile, slot, hypoth, kb_fact_id);
		if (slot->isBinaryRelation()) {
			createBinaryRelation(profile, slot, hypoth, kb_fact_id);
		}

	}

	// This should get called whether this is a new or old fact; there can always be new evidence
	addSupportToKBFact(kb_fact_id, hypoth);

	return kb_fact_id;
}

//
// Upload the hypothesis to the ProfileFact & BinaryRelation tables
//
int PGDatabaseManager::createProfileFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, int kb_fact_id) {

	// ProfileFact
	// profilefactid SERIAL NOT NULL,
	// actorid INT4 NOT NULL,
	// profiledisplayslotid INT4 NOT NULL,
	// displaystring TEXT NOT NULL,
	// displayactorid INT4 NOT NULL,
	// kbfactid INT4 NOT NULL,
	// createtime TIMESTAMP NOT NULL,
	// kbfactstatusid INT4 NOT NULL,

	std::wstring display_value = hypoth->getDisplayValue();
	int entity_request_id = -1;
	std::wstring entity_request_string = L"";

	if (NameHypothesis_ptr nameHypoth = boost::dynamic_pointer_cast<NameHypothesis>(hypoth)) {
		entity_request_id = nameHypoth->getActorId();
		entity_request_string = nameHypoth->getDisplayValue();
	}
	if (EmploymentHypothesis_ptr employHypoth = boost::dynamic_pointer_cast<EmploymentHypothesis>(hypoth)) {
		entity_request_id = employHypoth->getNamedArgument()->getActorId();
		entity_request_string = employHypoth->getNamedArgument()->getDisplayValue();
	}	
	if (DescriptionHypothesis_ptr descHypoth = boost::dynamic_pointer_cast<DescriptionHypothesis>(hypoth)) {
		display_value = profile->stripOfResolutionsToThisActor(display_value);
	}
	
	// This should already be done, but let's be very sure.
	display_value = OutputUtil::untokenizeString(display_value);	

	// Special for profile display items
	boost::replace_all(display_value, "&apos;&apos;", "\"");
	boost::replace_all(display_value, "&apos;", "'");
	boost::replace_all(display_value, "''", "\"");
	boost::replace_all(display_value, "``", "\"");
	
	std::wstringstream profileFactStream;
	profileFactStream << L"INSERT INTO ProfileFact (ActorId, ProfileDisplaySlotId, DisplayString, DisplayActorId, KBFactId, CreateTime, KBFactStatusId) VALUES (";
	profileFactStream << profile->getActorId() << L", "
				<< getDisplaySlotId(slot) << L", "
				<< L"'" << DatabaseConnection::sanitize(display_value) << L"',";
	if (entity_request_id == -1)
		profileFactStream << L"NULL,";
	else profileFactStream << entity_request_id << L",";
	profileFactStream << kb_fact_id << L",";
	profileFactStream << L"NOW(), ";	
	profileFactStream << _valid_fact_status_id;
	profileFactStream << ")";
	return insertReturningInt(profileFactStream.str(), L"ProfileFactId");	
}

int PGDatabaseManager::createBinaryRelation(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, int kb_fact_id) {

	// BinaryRelation
	// binaryrelationid SERIAL NOT NULL,
	// actorid1 INT4 NOT NULL,
	// actorid2 INT4 NOT NULL,
	// facttypeid INT4 NOT NULL,
	// roleid1 INT4 NOT NULL,
	// roleid2 INT4 NOT NULL,
	// kbfactid INT4 NOT NULL,
	// createtime TIMESTAMP NOT NULL,
	// kbfactstatusid INT4 NOT NULL,

	std::vector<GenericHypothesis::kb_arg_t> kb_actor_args;
	BOOST_FOREACH(GenericHypothesis::kb_arg_t kb_arg, hypoth->getKBArguments(profile->getActorId(), slot)) {
		if (kb_arg.actor_id != -1)
			kb_actor_args.push_back(kb_arg);
	}

	if (kb_actor_args.size() != 2)
		return -1;
	
	std::wstringstream binaryRelationStream;
	binaryRelationStream << L"INSERT INTO BinaryRelation (FactTypeId, ActorId1, RoleId1, ActorId2, RoleId2, KBFactId, CreateTime, KBFactStatusId) VALUES ("
		<< slot->getKBFactTypeId() << L", "
		<< kb_actor_args.at(0).actor_id << L", "
		<< convertStringToId("Role", kb_actor_args.at(0).role, true) << L", "
		<< kb_actor_args.at(1).actor_id << L", "
		<< convertStringToId("Role", kb_actor_args.at(1).role, true) << L", "
		<< kb_fact_id << L", "
		<< L"NOW(), "
		<< _valid_fact_status_id << ")";

	return insertReturningInt(binaryRelationStream.str(), L"BinaryRelationId");
}

//
// See if we already have a KB fact for this document fact
//
PGFact_ptr PGDatabaseManager::findMatchingKBFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth, std::vector<PGFact_ptr>& existingKBFacts) {

	std::vector<GenericHypothesis::kb_arg_t> kb_args = hypoth->getKBArguments(profile->getActorId(), slot);
	for (std::vector<PGFact_ptr>::iterator iter = existingKBFacts.begin(); iter != existingKBFacts.end(); iter++) {
		PGFact_ptr existingFact = (*iter);
		if (existingFact->getFactType() != slot->getKBFactTypeId())
			continue;
		if (existingFact->getAllArguments().size() != kb_args.size())
			continue;
		bool matched = false;			
		for (std::vector<GenericHypothesis::kb_arg_t>::iterator new_iter = kb_args.begin(); new_iter != kb_args.end(); new_iter++) {
			matched = false;
			for (std::vector<PGFactArgument_ptr>::iterator existing_iter = existingFact->getAllArguments().begin(); existing_iter != existingFact->getAllArguments().end(); existing_iter++) {
				if ((*existing_iter)->getRole() != (*new_iter).role)
					continue;
				if ((*new_iter).actor_id != -1) {
					if ((*existing_iter)->getActorId() == (*new_iter).actor_id)	{
						matched = true;
						break;
					}
				} else if ((*existing_iter)->getLiteralStringValue() == (*new_iter).value)	{
					matched = true;
					break;
				}
			}
			if (!matched)
				break;
		}
		if (matched)
			return existingFact;
	}

	return PGFact_ptr();
}


//
// Add/update supporting evidence to a KB fact (uniqueness constraint will prevent multiple entries)
//
void PGDatabaseManager::addSupportToKBFact(int kb_fact_id, GenericHypothesis_ptr hypoth) {	
	BOOST_FOREACH(PGFact_ptr fact, hypoth->getSupportingFacts()) {
		std::wstringstream itemStream;		
		if (fact->getDBFactType() == ProfileSlot::EXTERNAL) {			
			itemStream << L"INSERT INTO KbExternalSupportingFact (KbFactId, ExternalFactId) VALUES ("
				<< kb_fact_id << ", "
				<< fact->getFactId() << ")";	
		} else if (fact->getDBFactType() == ProfileSlot::DOCUMENT) {
			itemStream << L"INSERT INTO KbSupportingFact (KbFactId, FactId, DocumentId, PassageId, SourceLanguage, EvidenceText) VALUES ("
				<< kb_fact_id << ", "
				<< fact->getFactId() << ", "
				<< fact->getDocumentId() << ", "
				<< fact->getPassageId() << ", "
				<< "'" << fact->getLanguage().toShortString() << "', "
				<< "'" << UnicodeUtil::sqlEscapeApos(fact->getSupportChunkValue()) << "')";
		} else {
			// PGFact::FAKE
			continue;
		}

		try {
			_db->exec(itemStream.str());
		} catch (...) {
			// Failed because of uniqueness constraint (presumably); ignore
		}
	}
}

//
// Update an existing KB fact (with possibly new confidence and definitely new update time / epoch
//
void PGDatabaseManager::updateExistingKBFact(int kb_fact_id, double confidence, int fact_status_id, GenericHypothesis_ptr hypoth) {
	int supporting_fact_count = 0;

	// Should only be one of each of these, but we iterate anyway
	std::stringstream factCountStream;
	factCountStream << "select count(*) from kbsupportingfact where kbfactid = " << kb_fact_id;	
	for (DatabaseConnection::RowIterator row = _db->iter(factCountStream.str().c_str()); row!=_db->end(); ++row) {	
		supporting_fact_count += row.getCellAsInt32(0);
	}
	std::stringstream externalFactCountStream;
	externalFactCountStream << "select count(*) from kbexternalsupportingfact where kbfactid = " << kb_fact_id;	
	for (DatabaseConnection::RowIterator row = _db->iter(externalFactCountStream.str().c_str()); row!=_db->end(); ++row) {	
		supporting_fact_count += row.getCellAsInt32(0);
	}
	
	std::stringstream updateStream;
	updateStream << "UPDATE KbFact SET Confidence = " << confidence;
	updateStream << ", LastUpdateTime = NOW()";
	updateStream << ", LastUpdateEpoch = " << _epoch_id;
	updateStream << ", KBFactCount = " << supporting_fact_count;
	if (hypoth) {
		updateStream << ", EarliestDocumentTime = " << getDBDate(hypoth->getOldestCaptureTime()).c_str();
		updateStream << ", LatestDocumentTime = " << getDBDate(hypoth->getNewestCaptureTime()).c_str();
	}
	if (fact_status_id != -1) 
		updateStream << ", KBFactStatusId = " << fact_status_id;
	updateStream << " WHERE KbFactId = " << kb_fact_id;
	_db->exec(updateStream.str());
}

void PGDatabaseManager::markCacheEntryValid(std::string table_name, int item_id) {
	std::stringstream updateStream;
	updateStream << "UPDATE " << table_name << " SET KBFactStatusId = " << _valid_fact_status_id;
	updateStream << " WHERE " << table_name << "Id = " << item_id;
	_db->exec(updateStream.str());
}

//
// Create a new KB fact for a document fact
//
int PGDatabaseManager::createKBFact(Profile_ptr profile, ProfileSlot_ptr slot, GenericHypothesis_ptr hypoth) {

	std::stringstream insertStream;
	insertStream << "INSERT INTO KbFact (KbFactTypeId, Confidence, CreateTime, CreateEpoch, LastUpdateTime, LastUpdateEpoch, KBFactStatusId, EarliestDocumentTime, LatestDocumentTime) VALUES ("
			<< slot->getKBFactTypeId() << ", " << hypoth->getConfidence() << ", ";
	
	PGFact_ptr earliest = PGFact_ptr();
	if (_fake_kb_fact_creation_time) {
		BOOST_FOREACH(PGFact_ptr fact, hypoth->getSupportingFacts()) {
			if (fact->getDocumentDate().is_not_a_date())
				continue;
			if (earliest == PGFact_ptr() || fact->getDocumentDate() < earliest->getDocumentDate())
				earliest = fact;
		}
	}
	if (earliest) {
		insertStream << "'" << boost::gregorian::to_iso_extended_string(earliest->getDocumentDate()) << "', ";	
	} else insertStream << "NOW(), ";

	insertStream << _epoch_id << ", NOW(), " << _epoch_id << ", " << _valid_fact_status_id << ", ";
	insertStream << getDBDate(hypoth->getOldestCaptureTime()).c_str() << ", ";
	insertStream << getDBDate(hypoth->getNewestCaptureTime()).c_str();
	insertStream << ")";

	int kb_fact_id = insertReturningInt(insertStream.str(), "KbFactId");

	BOOST_FOREACH(GenericHypothesis::kb_arg_t kb_arg, hypoth->getKBArguments(profile->getActorId(), slot)) {
		addKBArgument(kb_fact_id, kb_arg);
	}

	return kb_fact_id;
}

//
// Add an argument to a KB fact
//
void PGDatabaseManager::addKBArgument(int kb_fact_id, GenericHypothesis::kb_arg_t kb_arg) {
	
	std::wstringstream insertStream;
	if (kb_arg.actor_id != -1) {
		insertStream << L"INSERT INTO KbFactArgument (KbFactId, RoleId, ActorId, StringValue) VALUES ("
					 << kb_fact_id << L","
					 << convertStringToId("Role", kb_arg.role, true) << L","
					 << kb_arg.actor_id << L","
					 << L"'" << DatabaseConnection::sanitize(kb_arg.value) << L"')";
	} else {		
		insertStream << L"INSERT INTO KbFactArgument (KbFactId, RoleId, StringValue) VALUES ("
					 << kb_fact_id << L","
					 << convertStringToId("Role", kb_arg.role, true) << L","
					 << L"'" << DatabaseConnection::sanitize(kb_arg.value) << L"')";
	}
	_db->exec(insertStream.str());

}

//
// Mark that a profile has been created/updated
//
void PGDatabaseManager::markActorProfileUpdated(int actor_id) {
	try {
		std::stringstream insertStream;
		insertStream << "INSERT INTO ActorProfileStatus (ActorId, UpToDate, CreateTime) VALUES (" << actor_id << ", TRUE, NOW() )";
		_db->exec(insertStream.str());
	} catch (...) {
		std::stringstream updateStream;
		updateStream << "UPDATE ActorProfileStatus SET UpToDate = TRUE, CreateTime = NOW() WHERE ActorId = " << actor_id;
		_db->exec(updateStream.str());
	}
}

//
// Mark profile out of date
//
void PGDatabaseManager::outDateProfile(Profile_ptr profile) {
	std::stringstream updateStream;
	updateStream << "UPDATE ActorProfileStatus SET UpToDate = 0 WHERE ActorId = " << profile->getActorId();
	_db->exec(updateStream.str());
}


//
// Return false if profile doesn't exist or is marked out of date
//
bool PGDatabaseManager::isUpToDate(int actor_id) {

	std::stringstream selectStream;
	selectStream << "SELECT UpToDate FROM ActorProfileStatus WHERE ActorId = " << actor_id;
		
	// Will return false if not in table
	bool up_to_date = false;
	for (DatabaseConnection::RowIterator row = _db->iter(selectStream.str().c_str()); row!=_db->end(); ++row) {	
		 if (row.getCellAsString(0) == "t") // this might be specific to postgres
			 up_to_date = true;
	}
	return up_to_date;

}

//
// Get actor strings (with confidences) for a given actor
//
PGDatabaseManager::actor_confidence_map_t PGDatabaseManager::getActorStringsForActor(int actor_id) {

	// We often make the same call many, many times for common actors; let's avoid this
	std::map<int, actor_confidence_map_t>::iterator iter = _actorStringCache.find(actor_id);
	if (iter != _actorStringCache.end()) {
		return (*iter).second;
	}		

	std::stringstream queryStream;
	queryStream << "SELECT String, Confidence FROM ActorString WHERE ActorId = " << actor_id;

	std::map<std::wstring, double> results;
	for (DatabaseConnection::RowIterator row = _db->iter(queryStream.str().c_str()); row!=_db->end(); ++row) {	
		std::wstring str = row.getCellAsWString(0);
		double confidence = row.getCellAsDouble(1);
		if (results.find(str) != results.end()) {
			if (results[str] < confidence)
				results[str] = confidence;
		} else {
			results[str] = confidence;
		}
	}
	_actorStringCache[actor_id] = results;
	return results;
}

void PGDatabaseManager::clearActorStringCache() {
	_actorStringCache.clear();
}


std::string PGDatabaseManager::getDBDate(boost::gregorian::date& d) {
	if (d.is_not_a_date())
		return "null";
	static const std::locale fmt(std::locale::classic(),
                      new boost::gregorian::date_facet("%Y-%m-%d"));
	std::ostringstream os;
    os.imbue(fmt);
    os << "'" << d << "'";
    return os.str();
}
