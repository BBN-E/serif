// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "ICEWS/ICEWSOutputWriter.h"
#include "ICEWS/EventMention.h"
#include "ICEWS/EventMentionSet.h"
#include "ICEWS/ActorMention.h"
#include "ICEWS/ActorInfo.h"
#include "ICEWS/Identifiers.h"
#include "ICEWS/SentenceSpan.h"
#include "ICEWS/ICEWSDB.h"
#include "ICEWS/Stories.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp> 

#ifdef _WIN32
#include <Windows.h>
#define sleep(x) Sleep((x)*1000)
#else
#include <unistd.h>
#endif

namespace {
	static const size_t NUM_RETRIES = 4;

	Symbol SOURCE_SYM(L"SOURCE");
	Symbol TARGET_SYM(L"TARGET");

	// Columns in our output table.  Note: if you change this, then
	// you must change the saveToDatabase() method accordingly!
	typedef const char *OutputTableColumn[2];
	OutputTableColumn CLASSIC_OUTPUT_TABLE_COLUMNS[] = {
		{"eventtype_id", "int(11) default NULL"},
		{"verbrule", "varchar(256) default NULL"},
		{"story_id", "int(11) default NULL"},
		{"sentence_num", "int(11) default NULL"},
		{"event_date", "date NOT NULL default '0000-01-01'"},
		{"coder_id", "int(10)  NOT NULL default '1'"},
		{"event_tense", "text"},
		// Event Participants:
		{"source_actor_id", "int(11) default NULL"},                 // null for composite actor
		{"source_actor_pattern_id", "int(11) default NULL"},         // null for composite actor
		{"source_paired_actor_id", "int(11) default NULL"},          // null for proper noun actor
		{"source_paired_agent_id", "int(11) default NULL"},          // null for proper noun actor
		{"source_paired_actor_pattern_id", "int(11) default NULL"},  // null for proper noun actor
		{"source_paired_agent_pattern_id", "int(11) default NULL"},  // null for proper noun actor
		{"target_actor_id", "int(11) default NULL"},                 // null for composite actor
		{"target_actor_pattern_id", "int(11) default NULL"},         // null for composite actor
		{"target_paired_actor_id", "int(11) default NULL"},          // null for proper noun actor
		{"target_paired_agent_id", "int(11) default NULL"},          // null for proper noun actor
		{"target_paired_actor_pattern_id", "int(11) default NULL"},  // null for proper noun actor
		{"target_paired_agent_pattern_id", "int(11) default NULL"}   // null for proper noun actor
	};
	
	OutputTableColumn WMS_OUTPUT_TABLE_COLUMNS[] = {
		{"event_code", "int(11) default NULL"},
		{"ss_id", "int(20) default NULL"},
		{"page_id", "int(11) default NULL"},
		{"page_version", "int(11) default NULL"},
		{"passage_id", "int(11) default NULL"},		
		{"event_tense", "text default NULL"},
		{"source_actor_id", "int(11) default NULL"},
		{"source_actor_name", "varchar(256) default NULL"},
		{"source_country_id", "int(11) default NULL"},
		{"source_country_name", "varchar(256) default NULL"},
		{"source_agent_id", "int(11) default NULL"},
		{"source_agent_name", "varchar(256) default NULL"},
		{"target_actor_id", "int(11) default NULL"},
		{"target_actor_name", "varchar(256) default NULL"},
		{"target_country_id", "int(11) default NULL"},
		{"target_country_name", "varchar(256) default NULL"},
		{"target_agent_id", "int(11) default NULL"},
		{"target_agent_name", "varchar(256) default NULL"},
		{"event_location_id", "int(11) default NULL"},
		{"event_location_latitude", "int(11) default NULL"},
		{"event_location_longitude", "int(11) default NULL"},
		{"event_location_name", "int(11) default NULL"},
		{"event_location_country", "int(11) default NULL"}		
	};


}

namespace ICEWS {

	ICEWSOutputWriter::ICEWSOutputWriter() {
	_db_table_name = ParamReader::getParam("icews_save_events_to_database_table");
	_check_for_duplicates = ParamReader::isParamTrue("icews_check_for_duplicate_rows_when_writing_to_database");
	_coder_id = ParamReader::getOptionalIntParamWithDefaultValue("icews_coder_id", 2);

	_output_type = CLASSIC_ICEWS;
	_NUM_COLUMNS = sizeof(CLASSIC_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	if (ParamReader::isParamTrue("run_icews_for_wms")) {
		_output_type = WMS;
		_NUM_COLUMNS = sizeof(WMS_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	}
}

ICEWSOutputWriter::~ICEWSOutputWriter() {}

void ICEWSOutputWriter::process(DocTheory* docTheory) {
	if (!_db_table_name.empty()) {
		ensureOutputTableExists();
		ICEWSEventMentionSet* eventMentionSet = docTheory->getSubtheory<ICEWSEventMentionSet>();
		if (eventMentionSet) {
			BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentionSet)) {
				if (em->isDatabaseWorthyEvent())
					saveToDatabase(em, docTheory);
			}
			SessionLogger::info("ICEWS") << "Saved " << eventMentionSet->size() 
										 << " events to " << _db_table_name;
		}
	}
}

void ICEWSOutputWriter::saveToDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory) {

	// Get the participant identifiers.
	ActorMention_ptr sourceActor;
	ActorMention_ptr targetActor;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantMap()) {
		if (participantPair.first == TARGET_SYM) {
			targetActor = participantPair.second;
		} else if (participantPair.first == SOURCE_SYM) {
			sourceActor = participantPair.second;
		} else {
			throw UnexpectedInputException("ICEWSEventMention::saveToDatabase",
				"EventMention has non-standard participant role: ", 
				participantPair.first.to_debug_string());
		}
	}

	if (_output_type == CLASSIC_ICEWS)
		saveToClassicDatabase(em, sourceActor, targetActor, docTheory);
	else if (_output_type == WMS)
		saveToWMSDatabase(em, sourceActor, targetActor, docTheory);
}


void ICEWSOutputWriter::ensureOutputTableExists() {
	static bool completed = false;
	if (completed) return;

	if (!_db_table_name.empty()) {
		DatabaseConnection_ptr icews_db(getSingletonIcewsOutputDb());
		bool oracleDb = icews_db->getSqlVariant() == "Oracle";
		if (oracleDb) {
			std::ostringstream existsQuery;
			std::cerr << "Checking if table exists!" << std::endl;
			existsQuery << "select count(*) from user_tables where "
						<< "table_name = " << icews_db->quote(_db_table_name);
			if (icews_db->iter(existsQuery).getCellAsInt32(0) != 0) {
				std::cerr << "  Exists already -- exiting." << std::endl;
				return; // Table already exists.
			}
			std::cerr << "  Does not exist yet -- creating." << std::endl;
		}
		std::ostringstream query;
		query << "CREATE TABLE ";
		if (!oracleDb)
			query << "IF NOT EXISTS " ;
		query << _db_table_name << " (";
		for (size_t i=0; i<_NUM_COLUMNS; ++i) {
			if (i>0) query << ",";
			if (_output_type == CLASSIC_ICEWS)
				query << CLASSIC_OUTPUT_TABLE_COLUMNS[i][0] << " " << CLASSIC_OUTPUT_TABLE_COLUMNS[i][1];
			else if (_output_type == WMS)
				query << WMS_OUTPUT_TABLE_COLUMNS[i][0] << " " << WMS_OUTPUT_TABLE_COLUMNS[i][1];
		}
		query << ");";
		icews_db->exec(query);
		SessionLogger::info("ICEWS") << "Created table " << _db_table_name;
	}
	completed = true;
}

/** Add the names of the columns that are used to store a given
	participant (source or target).  Include the leading comma.  */
void ICEWSOutputWriter::addClassicParticipantColumnNamesToQuery(const char* role, std::ostringstream &query) {
	query << ","
		  << role << "_actor_id," << role << "_actor_pattern_id," 
		  << role << "_paired_actor_id," << role << "_paired_agent_id,"
		  << role << "_paired_actor_pattern_id," << role << "_paired_agent_pattern_id";
}

/** Add the cell values that are used to store a given participant
	(source or target).  Include the leading comma.  */
void ICEWSOutputWriter::addClassicParticipantIdsToQuery(ActorMention_ptr actor, const char* role, std::ostringstream &query) {
	query << ",";
	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
		query << pm->getActorId() << ", " << pm->getActorPatternId() 
			  << ", NULL, NULL, NULL, NULL";
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
		query << "NULL, NULL, " << cm->getPairedActorId() << ", " << cm->getPairedAgentId() << ", "
			  << cm->getPairedActorPatternId() << ", " << cm->getPairedAgentPatternId();
	} else {
		query << "NULL, NULL, NULL, NULL, NULL, NULL";
	}
}


void ICEWSOutputWriter::saveToClassicDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory) 
{
	ICEWSEventTypeId eventtype_id = em->getEventType()->getEventId();
	
	// Use the pattern id as the "verbrule" string:
	std::string verbrule = UnicodeUtil::toUTF8StdString(em->getPatternId().to_string());

	// Get the story id.
	StoryId storyId = Stories::extractStoryIdFromDocId(docTheory->getDocument()->getName());
	if (storyId.isNull()) {
		throw UnexpectedInputException("ICEWSOutputWriter::saveToClassicDatabase",
			"Unable to extract story id from docid: ", docTheory->getDocument()->getName().to_debug_string());
	}

	// Determine the sentence number.  Note that SERIF sentences and ICEWS sentences
	// may not always be the same.  Display a warning if any participant falls 
	// outside the ICEWS sentence we identified.
	int serif_sent_no = sourceActor->getEntityMention()->getSentenceNumber();
	std::set<int> participant_sent_number_set;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	typedef std::pair<ParticipantPair, int> ParticipantSentNumPair;
	std::vector<ParticipantSentNumPair> participant_sent_numbers;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantMap()) {
		const Mention* m = participantPair.second->getEntityMention();
		const SentenceTheory* sentTheory = docTheory->getSentenceTheory(m->getSentenceNumber());
		int tok = m->getAtomicHead()->getStartToken();
		EDTOffset offset = sentTheory->getTokenSequence()->getToken(tok)->getStartEDTOffset();
		int icews_sent_no = IcewsSentenceSpan::edtOffsetToIcewsSentenceNo(offset, docTheory);
		participant_sent_number_set.insert(icews_sent_no);
		participant_sent_numbers.push_back(ParticipantSentNumPair(participantPair, icews_sent_no));
	}
	if (participant_sent_numbers.size() == 0) {
		throw InternalInconsistencyException("ICEWSEventMention::saveToClassicDatabase", 
			"ICEWS event has no participants");
	} else if (participant_sent_number_set.size() > 1) {
		std::ostringstream err;
		err << "ICEWS Event Participants come from different ICEWS sentences:\n";
		BOOST_FOREACH(ParticipantSentNumPair p, participant_sent_numbers) {
			err << "  * [icews_sentno=" << p.second << "] " << p.first.first << " = \"" 
				<< p.first.second->getEntityMention()->toCasedTextString() << "\"\n";
		}
		SessionLogger::warn("ICEWS") << err.str().c_str();
	}
	int icews_sent_no = *(participant_sent_number_set.begin());

	std::string event_text = UnicodeUtil::toUTF8StdString(em->getEventText());

	// Use the story publication date as the event date.
	std::string event_date;
	const LocatedString *dateTimeField = docTheory->getDocument()->getDateTimeField();
	if (dateTimeField)
		event_date = UnicodeUtil::toUTF8StdString(dateTimeField->toWString());
	else
		event_date = Stories::getStoryPublicationDate(docTheory->getDocument());

	DatabaseConnection_ptr icews_db(getSingletonIcewsOutputDb());

	std::string event_tense(UnicodeUtil::toUTF8StdString(em->getEventTense().to_string()));

	std::ostringstream query;
	query << "INSERT INTO " << _db_table_name << " ("
		  << " eventtype_id, verbrule, story_id, sentence_num, event_date, coder_id, event_tense";
	addClassicParticipantColumnNamesToQuery("source", query);
	addClassicParticipantColumnNamesToQuery("target", query);
	query << ") VALUES ("
		  << eventtype_id << ", " 
		  //<< DatabaseConnection::quote(verbrule) << ", " 
		  << "NULL, " 
		  << storyId << ", "
		  << icews_sent_no << ", " 
		  << DatabaseConnection::quote(event_date) << ", " 
		  << _coder_id << ", " 
		  << DatabaseConnection::quote(event_tense);
	addClassicParticipantIdsToQuery(sourceActor, "source", query);
	addClassicParticipantIdsToQuery(targetActor, "target", query);
	query << ")";
	try {
		icews_db->exec(query);
	} catch (UnexpectedInputException &) {
		// Database might be locked; try again.
		unsigned int delay = 1; // one second
		for (size_t retryNum=0; retryNum<NUM_RETRIES; retryNum++) {
			SessionLogger::warn("ICEWS") << "Unable to write to database; "
										 << "retrying in " << delay << "seconds";
			sleep(delay);
			delay *= 2;
			try {
				icews_db->exec(query);
				return;
			} catch (UnexpectedInputException &) {
				// try again.
			}
		}

	}
}

/** Add the cell values that are used to store a given participant
	(source or target) for WMS.  Include the leading comma.  */
void ICEWSOutputWriter::addWMSParticipantToQuery(ActorMention_ptr actor, std::ostringstream &query) {

	boost::shared_ptr<ActorInfo> actorInfo = ActorInfo::getActorInfoSingleton();

	query << ",";
	ActorId actorID = ActorId();
	AgentId agentID = AgentId();

	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
		actorID = pm->getActorId();
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
		actorID = cm->getPairedActorId();
		agentID = cm->getPairedAgentId();
	} 

	std::string actorName = "NULL";

	if (!actorID.isNull()) {
		query << actorID << ", ";
		query << DatabaseConnection::quote(actorInfo->getName(actorID)) << ", ";
		std::vector<ActorId> countries = actorInfo->getAssociatedCountryActorIds(actorID);
		if (countries.size() > 0) {
			ActorId countryID = countries.at(0);
			query << countryID << ", ";
			query << DatabaseConnection::quote(actorInfo->getName(countryID)) << ", ";
		} else query << "NULL, NULL, ";
	} else query << "NULL, NULL, NULL, NULL, ";

	if (!agentID.isNull()) {
		query << agentID << ", ";
		query << DatabaseConnection::quote(actorInfo->getName(agentID));
	} else query << "NULL, NULL ";
}

/** Add the cell values that are used to store a location for WMS.  Include the leading comma.  */
void ICEWSOutputWriter::addWMSLocationToQuery(ActorMention_ptr actor, std::ostringstream &query) {

	// TODO
	query << ", NULL, NULL, NULL, NULL, NULL";
}


void ICEWSOutputWriter::saveToWMSDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory) 
{
	std::wstring event_code = em->getEventType()->getEventCode().to_string();
	std::wstring ss_id;
	std::wstring page_id;
	std::wstring page_version;

	ActorMention_ptr locationActor = ActorMention_ptr();
	
	static const boost::wregex DOCID_RE_WMS(L"[a-z]+-[a-z]+-[a-z]+-[a-z]+-[A-Z][A-Z][A-Z][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]_([0-9]+)\\.([0-9]+)\\.([0-9]+)\\..*");
	boost::wsmatch match;
	std::wstring docid_str(docTheory->getDocument()->getName().to_string());
	if (boost::regex_match(docid_str, match, DOCID_RE_WMS)) {
		ss_id = match.str(1);
		page_id = match.str(2);
		page_version = match.str(3);
	} else {
		throw UnexpectedInputException("ICEWSOutputWriter::saveToWMSDatabase",
			"Unable to extract ss/page/version information from docid: ", docTheory->getDocument()->getName().to_debug_string());
	}

	int passage_id = OutputUtil::convertSerifSentenceToPassageId(docTheory, sourceActor->getEntityMention()->getSentenceNumber());
	std::string event_tense(UnicodeUtil::toUTF8StdString(em->getEventTense().to_string()));

	DatabaseConnection_ptr icews_db(getSingletonIcewsOutputDb());

	std::ostringstream query;
	query << "INSERT INTO " << _db_table_name << " ("
		  << " event_code, ss_id, page_id, page_version, passage_id, event_tense,"
		  << " source_actor_id, source_actor_name, source_country_id, source_country_name, source_agent_id, source_agent_name, "
		  << " target_actor_id, target_actor_name, target_country_id, target_country_name,target_agent_id, target_agent_name, "
		  << " event_location_id, event_location_latitude, event_location_longitude, event_location_name, event_location_country";
	query << ") VALUES ("
		  << event_code << ", " 
		  << ss_id << ", "
		  << page_id << ", "
		  << page_version << ", "
		  << passage_id << ", "
		  << DatabaseConnection::quote(event_tense);
	addWMSParticipantToQuery(sourceActor, query);
	addWMSParticipantToQuery(targetActor, query);
	addWMSLocationToQuery(locationActor, query);
	query << ")";
	icews_db->exec(query);	
}


} // end of namespace
