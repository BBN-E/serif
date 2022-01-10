// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/icews/ICEWSOutputWriter.h"
#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/icews/ICEWSEventMentionSet.h"
#include "Generic/icews/SentenceSpan.h"
#include "Generic/icews/Stories.h"
#include "Generic/icews/ICEWSDB.h"
#include "Generic/icews/ICEWSActorInfo.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorInfo.h"
#include "Generic/theories/DocTheory.h"
#include "Generic/theories/Document.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Token.h"
#include "Generic/theories/ActorMention.h"
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>


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
	Symbol LOCATION_SYM(L"LOCATION");

	Symbol ARTIFACT_SYM(L"ARTIFACT");
	Symbol INFORMATION_SYM(L"INFORMATION");

	Symbol ICEWS_SYM(L"ICEWS");
	Symbol WMS_SYM(L"WMS");
	Symbol CWMD_SYM(L"CWMD");
	Symbol SIMPLE_SYM(L"SIMPLE");

	// Columns in our output table.  Note: if you change this, then
	// you must change the saveToDatabase() method accordingly!
	// Fields: name, sqlite-declaration, oracle-declaration
	typedef const char *OutputTableColumn[3];
	static const size_t NAME_FIELD = 0;
	static const size_t SQLITE_DECL_FIELD = 1;
	static const size_t ORACLE_DECL_FIELD = 2;
	OutputTableColumn CLASSIC_OUTPUT_TABLE_COLUMNS[] = {
		{"eventtype_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"verbrule", "varchar(256) default NULL", "VARCHAR2(1000) default NULL"},
		{"story_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"sentence_num", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"event_date", "date NOT NULL default '0000-01-01'", "VARCHAR2(100) NOT NULL"},
		{"coder_id", "int(10)  NOT NULL default '1'", "NUMBER(10, 0)  default '1' NOT NULL"},
		{"event_tense", "text", "CLOB"},
		// Event Participants:
		{"source_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},                 // null for composite actor
		{"source_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},         // null for composite actor
		{"source_paired_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"source_paired_agent_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"source_paired_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"source_paired_agent_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"target_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},                 // null for composite actor
		{"target_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},         // null for composite actor
		{"target_paired_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"target_paired_agent_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"target_paired_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"target_paired_agent_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"}   // null for proper noun actor
	};

	OutputTableColumn SIMPLE_OUTPUT_TABLE_COLUMNS[] = {
		{"event_type", "text", "NO_ORACLE_DECL_DEFINED_YET"},
		{"document_id", "text", "NO_ORACLE_DECL_DEFINED_YET"},
		{"sentence_id", "int(11)", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_tense", "text", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_actor_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_actor_name", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_country_codes", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_agent_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_agent_name", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_actor_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_actor_name", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_country_codes", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_agent_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_agent_name", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
	};
	
	OutputTableColumn WMS_OUTPUT_TABLE_COLUMNS[] = {
		{"event_code", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"ss_id", "int(20) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"page_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"page_version", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"passage_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},		
		{"event_tense", "text default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_actor_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_actor_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_country_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_country_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_agent_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"source_agent_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_actor_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_actor_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_country_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_country_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_agent_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"target_agent_name", "varchar(256) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_location_id", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_location_latitude", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_location_longitude", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_location_name", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"},
		{"event_location_country", "int(11) default NULL", "NO_ORACLE_DECL_DEFINED_YET"}		
	};

	OutputTableColumn CWMD_OUTPUT_TABLE_COLUMNS[] = {
		{"eventtype_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"verbrule", "varchar(256) default NULL", "VARCHAR2(1000) default NULL"},
		{"story_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"sentence_num", "int(11) default NULL", "NUMBER(10, 0) default NULL"},
		{"event_date", "date NOT NULL default '0000-01-01'", "VARCHAR2(100) NOT NULL"},
		{"coder_id", "int(10)  NOT NULL default '1'", "NUMBER(10, 0)  default '1' NOT NULL"},
		{"event_tense", "text", "CLOB"},
		// Event Participants:
		{"source_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},                 // null for composite actor
		{"source_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},         // null for composite actor
		{"source_paired_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"source_paired_agent_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"source_paired_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"source_paired_agent_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"target_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},                 // null for composite actor
		{"target_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},         // null for composite actor
		{"target_paired_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"target_paired_agent_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},          // null for proper noun actor
		{"target_paired_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"target_paired_agent_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},  // null for proper noun actor
		{"artifact_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},               // null for composite actor
		{"artifact_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},       // null for composite actor
		{"artifact_paired_actor_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},        // null for proper noun actor
		{"artifact_paired_agent_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},        // null for proper noun actor
		{"artifact_paired_actor_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"},// null for proper noun actor
		{"artifact_paired_agent_pattern_id", "int(11) default NULL", "NUMBER(10, 0) default NULL"}// null for proper noun actor
	};
	
}
ICEWSOutputWriter::ICEWSOutputWriter() {
	_db_table_name = ParamReader::getParam("icews_save_events_to_database_table");
	_check_for_duplicates = ParamReader::isParamTrue("icews_check_for_duplicate_rows_when_writing_to_database");
	_coder_id = ParamReader::getOptionalIntParamWithDefaultValue("icews_coder_id", 2);
	_output_format = Symbol(UnicodeUtil::toUTF16StdString(ParamReader::getRequiredParam("icews_output_format")));
	_actorInfo = ActorInfo::getAppropriateActorInfoForICEWS();

	if (_output_format == ICEWS_SYM) {
		_NUM_COLUMNS = sizeof(CLASSIC_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	} else if (_output_format == WMS_SYM) {
		_NUM_COLUMNS = sizeof(WMS_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	} else if (_output_format == CWMD_SYM) {
		_NUM_COLUMNS = sizeof(CWMD_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	} else if (_output_format == SIMPLE_SYM) {
		_NUM_COLUMNS = sizeof(SIMPLE_OUTPUT_TABLE_COLUMNS)/sizeof(OutputTableColumn);
	} else {
		std::stringstream err;
		err << "Parameter 'icews_output_format' must be set to 'ICEWS', 'SIMPLE', 'WMS' or 'CWMD'";
		throw new UnexpectedInputException("ICEWSOutputWriter::ICEWSOutputWriter()", err.str().c_str());
	}
}

ICEWSOutputWriter::~ICEWSOutputWriter() {}

void ICEWSOutputWriter::process(DocTheory* docTheory) {
	if (_output_format == WMS_SYM) {
		processForWMS(docTheory);
	}
	else if (!_db_table_name.empty()) {
		ensureOutputTableExists();
		ICEWSEventMentionSet* eventMentionSet = docTheory->getICEWSEventMentionSet();
		if (eventMentionSet) {
			int count = 0;
			BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentionSet)) {
				if (em->isDatabaseWorthyEvent()) {
					saveToDatabase(em, docTheory);
					count++;
				} else {
					SessionLogger::info("ICEWS") << "Discarding event as not database-worthy\n";
				}
			}
			SessionLogger::info("ICEWS") << "Saved " << count
										 << " events to " << _db_table_name;
		}
	}
}

////////////////////////////////////////////////////
/* WMS OUTPUT FUNCTIONS							  */
////////////////////////////////////////////////////

void ICEWSOutputWriter::processForWMS(DocTheory* docTheory) {

	//get doc id
	std::wstring docid_wstr(docTheory->getDocument()->getName().to_string());
	std::string docid_str(docid_wstr.begin(), docid_wstr.end());
	boost::filesystem::path full_doc_path(docid_str);
	boost::filesystem::path doc_fname = full_doc_path.filename();
	boost::filesystem::path out_file_path = doc_fname;
	out_file_path.replace_extension("xml");
	//get output folder path
	boost::filesystem::path output(ParamReader::getParam("icews_wms_output_folder"));
	if (!boost::filesystem::is_directory(output)) {
		boost::filesystem::create_directories(output);
		SessionLogger::info("ICEWS") << "Created ICEWS output directory " << output.string();
	}
	output /= out_file_path;
	//open up file in folder
	boost::filesystem::ofstream outfile;
	outfile.open (output);

	ICEWSEventMentionSet* eventMentionSet = docTheory->getICEWSEventMentionSet();
	ICEWSDocData docData = extractDocumentData(docTheory);
	outfile << "<events>" << std::endl;
	outfile << docDataToXML(docData);
	if (eventMentionSet) {
		BOOST_FOREACH(ICEWSEventMention_ptr em, (*eventMentionSet)) {
			if (em->isDatabaseWorthyEvent()){
				ICEWSEventData eventData = extractEventData(em, docTheory);
				outfile << eventToXML(eventData);
			}
		}
		SessionLogger::info("ICEWS") << "Saved SQL for " << eventMentionSet->size() 
									 << " events to " << _db_table_name;
	}
	outfile << "</events>" << std::endl;
	outfile.close();
}

std::wstring ICEWSOutputWriter::docDataToXML(ICEWSDocData docData, std::wstring indent /* defaults to '  ' */) {
	std::wstringstream xml;
	xml << indent << L"<wms_ss_id>" << docData.wms_ss_id << L"</wms_ss_id>" << std::endl;
	xml << indent << L"<wms_page_id>" << docData.wms_page_id << L"</wms_page_id>" << std::endl;
	return xml.str();
}

std::wstring ICEWSOutputWriter::eventToXML(ICEWSEventData eventData, std::wstring indent /* defaults to '\t' */){
	std::wstringstream xml;
	xml << indent << L"<event>" << std::endl;
	xml << indent << indent << L"<wms_passage_id>" << eventData.wms_passage_id << L"</wms_passage_id>" << std::endl;
	xml << indent << indent << L"<event_code>" << eventData.event_code << L"</event_code>" << std::endl;
	xml << indent << indent << L"<event_tense>" << eventData.event_tense << L"</event_tense>" << std::endl;
	xml << indent << indent << L"<source_actor_id>" << eventData.source_actor_id << L"</source_actor_id>" << std::endl;
	xml << indent << indent << L"<source_actor_name>" << eventData.source_actor_name << L"</source_actor_name>" << std::endl;
	xml << indent << indent << L"<source_agent_id>" << eventData.source_agent_id << L"</source_agent_id>" << std::endl;
	xml << indent << indent << L"<source_agent_name>" << eventData.source_agent_name << L"</source_agent_name>" << std::endl;
	xml << indent << indent << L"<source_country_code>" << eventData.source_country_code << L"</source_country_code>" << std::endl;
	xml << indent << indent << L"<target_actor_id>" << eventData.target_actor_id << L"</target_actor_id>" << std::endl;
	xml << indent << indent << L"<target_actor_name>" << eventData.target_actor_name << L"</target_actor_name>" << std::endl;
	xml << indent << indent << L"<target_agent_id>" << eventData.target_agent_id << L"</target_agent_id>" << std::endl;
	xml << indent << indent << L"<target_agent_name>" << eventData.target_agent_name << L"</target_agent_name>" << std::endl;
	xml << indent << indent << L"<target_country_code>" << eventData.target_country_code << L"</target_country_code>" << std::endl;
	xml << indent << indent << L"<location_name>" << eventData.location_name << L"</location_name>" << std::endl;
	xml << indent << indent << L"<location_country_code>" << eventData.location_country_code << L"</location_country_code>" << std::endl;
	xml << indent << indent << L"<location_latitude>" << eventData.location_latitude << L"</location_latitude>" << std::endl;
	xml << indent << indent << L"<location_longitude>" << eventData.location_longitude << L"</location_longitude>" << std::endl;
	xml << indent << indent << L"<event_date>" << eventData.event_date << L"</event_date>" << std::endl;	
	xml << indent << L"</event>" << std::endl;
	return xml.str();
}

ICEWSOutputWriter::ICEWSEventData ICEWSOutputWriter::extractEventData(ICEWSEventMention_ptr em, const DocTheory* docTheory) { 
	//initialize main datastructure
	ICEWSEventData eventData;

	// get generic event data
	eventData.event_date = L"";
	Symbol event_tense = em->getEventTense();
	if(!event_tense.is_null())
		eventData.event_tense = event_tense.to_string();
	Symbol event_code = em->getEventType()->getEventCode();
	if(!event_code.is_null())
		eventData.event_code = event_code.to_string();
	else
		throw UnexpectedInputException("ICEWSEventMention::extractEventData", 
				"EventMention has null event code: ", 
				event_code.to_debug_string());


	// get the participant identifiers.
	ActorMention_ptr sourceActor;
	ActorMention_ptr targetActor;
	ProperNounActorMention_ptr pnLocationActor;
	bool hasLocation = false;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantList()) {
		if (participantPair.first == TARGET_SYM) {
			targetActor = participantPair.second;
		} else if (participantPair.first == SOURCE_SYM) {
			sourceActor = participantPair.second;
		} else if (participantPair.first == LOCATION_SYM) {
			hasLocation = true;
			pnLocationActor = boost::dynamic_pointer_cast<ProperNounActorMention>(participantPair.second);
		} else if (participantPair.first == ARTIFACT_SYM || participantPair.first == INFORMATION_SYM) {
			// Ignore for now, since we don't represent artifacts or information in XML yet
			continue;
		} else {
			throw UnexpectedInputException("ICEWSEventMention::saveToDatabase",
				"EventMention has non-standard participant role: ", 
				participantPair.first.to_debug_string());
		}
	}

	// get passage id using sourceActor
	eventData.wms_passage_id = boost::lexical_cast<std::wstring>(OutputUtil::convertSerifSentenceToPassageId(docTheory, sourceActor->getEntityMention()->getSentenceNumber()));

	// get event location 
	if (hasLocation) 
		extractEventLocationData(pnLocationActor, eventData);

	// get source data
	extractEventSourceData(sourceActor, eventData);

	// get target data 
	extractEventTargetData(targetActor, eventData);

	return eventData;
}

// add document id related data (ss_id, page_id, etc.) to ICEWSEventData struct
ICEWSOutputWriter::ICEWSDocData ICEWSOutputWriter::extractDocumentData(const DocTheory* docTheory) {
	std::wstring ss_id;
	std::wstring page_id; 

	//// when testing WMS output on docs not from WMS, uncomment the following lines
	//ss_id = L"128033024132";
	//page_id = L"1";

	static const boost::wregex DOCID_RE_WMS(L"[a-z]+-[a-z]+-[a-z]+-[a-z]+-[A-Z][A-Z][A-Z][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]_([0-9]+)\\.([0-9]+)\\..*");
	//static const boost::wregex DOCID_RE_WMS(L"[a-z]+-[a-z]+-[a-z]+-[a-z]+-[A-Z][A-Z][A-Z][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]_([0-9]+)\\.([0-9]+)\\.([0-9]+)\\..*");
	boost::wsmatch match;
	std::wstring docid_str(docTheory->getDocument()->getName().to_string());
	if (boost::regex_match(docid_str, match, DOCID_RE_WMS)) {
		ss_id = match.str(1);
		page_id = match.str(2);
	} else {
		throw UnexpectedInputException("ICEWSOutputWriter::saveToWMSDatabase",
			"Unable to extract ss/page/version information from docid: ", docTheory->getDocument()->getName().to_debug_string());
	}

	ICEWSDocData docData;
	docData.wms_ss_id = ss_id;
	docData.wms_page_id = page_id;

	return docData;
}

// add event location data (latitude, longitude, etc.) to ICEWSEventData struct
void ICEWSOutputWriter::extractEventLocationData(ProperNounActorMention_ptr locationActor, ICEWSEventData& eventData) {
	if (locationActor && locationActor->isResolvedGeo())
	{
		// add country code to data, must be present
		Symbol countrycode = locationActor->getGeoResolution()->countrycode;
		if(!countrycode.is_null())
			eventData.location_country_code = countrycode.to_string();
		else
			throw UnexpectedInputException("ICEWSEventMention::extractLocationData", 
					"Event location ActorMention has null country code: ", 
					countrycode.to_debug_string());
		// if latitude / longitude are NaN, leave these fields as empty string
		if(locationActor->getGeoResolution()->latitude != std::numeric_limits<float>::quiet_NaN()) 
			eventData.location_latitude = boost::lexical_cast<std::wstring>(locationActor->getGeoResolution()->latitude);
		if(locationActor->getGeoResolution()->longitude != std::numeric_limits<float>::quiet_NaN()) 
			eventData.location_longitude = boost::lexical_cast<std::wstring>(locationActor->getGeoResolution()->longitude);
		// add city name for geo, if null leave as empty string
		Symbol cityname = locationActor->getGeoResolution()->cityname;
		if (!cityname.is_null())
			eventData.location_name = cityname.to_string();
	}
}


// add event source actor data (actor_id, actor_name, etc.) to ICEWSEventData struct
void ICEWSOutputWriter::extractEventSourceData(ActorMention_ptr actor, ICEWSEventData& eventData) {
	ActorId actorID = ActorId();
	AgentId agentID = AgentId();

	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
		actorID = pm->getActorId();
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
		actorID = cm->getPairedActorId();
		agentID = cm->getPairedAgentId();
	} 

	if (!actorID.isNull()) {
		eventData.source_actor_id = boost::lexical_cast<std::wstring>(actorID.getId());
		eventData.source_actor_name = _actorInfo->getName(actorID);
		std::vector<ActorId> countries = _actorInfo->getAssociatedCountryActorIds(actorID);
		if (countries.size() > 0) {
			eventData.source_country_code = L"";
			eventData.source_country_codes = L"";
			BOOST_FOREACH(ActorId countryId, countries) {
				Symbol cc = _actorInfo->getIsoCodeForActor(countryId);
				if (!cc.is_null()) {
					if (eventData.source_country_code == L"") {
						eventData.source_country_code = cc.to_string();
					}
					if (eventData.source_country_codes != L"") {
						eventData.source_country_codes += L",";
					}
					eventData.source_country_codes += cc.to_string();
				}
			}			
		} 
	}

	if (!agentID.isNull()) {
		eventData.source_agent_id = boost::lexical_cast<std::wstring>(agentID.getId());
		eventData.source_agent_name = _actorInfo->getName(agentID);
	}
}

// add event target actor data (actor_id, actor_name, etc.) to ICEWSEventData struct 
void ICEWSOutputWriter::extractEventTargetData(ActorMention_ptr actor, ICEWSEventData& eventData) {
	ActorId actorID = ActorId();
	AgentId agentID = AgentId();

	if (ProperNounActorMention_ptr pm = boost::dynamic_pointer_cast<ProperNounActorMention>(actor)) {
		actorID = pm->getActorId();
	} else if (CompositeActorMention_ptr cm = boost::dynamic_pointer_cast<CompositeActorMention>(actor)) {
		actorID = cm->getPairedActorId();
		agentID = cm->getPairedAgentId();
	} 

	if (!actorID.isNull()) {
		eventData.target_actor_id = boost::lexical_cast<std::wstring>(actorID.getId());
		eventData.target_actor_name = _actorInfo->getName(actorID);
		std::vector<ActorId> countries = _actorInfo->getAssociatedCountryActorIds(actorID);
		if (countries.size() > 0) {
			eventData.target_country_code = L"";
			eventData.target_country_codes = L"";
			BOOST_FOREACH(ActorId countryId, countries) {
				Symbol cc = _actorInfo->getIsoCodeForActor(countryId);
				if (!cc.is_null()) {
					if (eventData.target_country_code == L"") {
						eventData.target_country_code = cc.to_string();
					}
					if (eventData.target_country_codes != L"") {
						eventData.target_country_codes += L",";
					}
					eventData.target_country_codes += cc.to_string();
				}
			}			
		}
	}

	if (!agentID.isNull()) {
		eventData.target_agent_id = boost::lexical_cast<std::wstring>(agentID.getId());
		eventData.target_agent_name = _actorInfo->getName(agentID);
	}
}

////////////////////////////////////////////////////
/* OLDER WMS FUNCTIONS --> TO BE CLEANED UP LATER */
////////////////////////////////////////////////////

void ICEWSOutputWriter::saveToDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory) 
{
	if (_output_format == SIMPLE_SYM) {
		saveToSimpleDatabase(em, docTheory);
		return;
	}

	// Get the participant identifiers.
	ActorMention_ptr sourceActor;
	ActorMention_ptr targetActor;
	ActorMention_ptr artifactActor;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantList()) {
		if (participantPair.first == TARGET_SYM) {
			targetActor = participantPair.second;
		} else if (participantPair.first == SOURCE_SYM) {
			sourceActor = participantPair.second;
		} else if (participantPair.first == ARTIFACT_SYM) {
			artifactActor = participantPair.second;
		} else if (participantPair.first == LOCATION_SYM || participantPair.first == INFORMATION_SYM) {
			continue;
		} else {
			throw UnexpectedInputException("ICEWSEventMention::saveToDatabase",
				"EventMention has non-standard participant role: ", 
				participantPair.first.to_debug_string());
		}
	}

	if (_output_format == ICEWS_SYM || _output_format == CWMD_SYM)
		saveToDatabase(em, sourceActor, targetActor, artifactActor, docTheory);
	else if (_output_format == WMS_SYM)
		saveToWMSDatabase(em, sourceActor, targetActor, docTheory);
}


void ICEWSOutputWriter::ensureOutputTableExists() {
	static bool completed = false;
	if (completed) return;

	if (ParamReader::isParamTrue("icews_skip_ensure_output_table_exists")) {
		completed = true;
		return;
	}

	if (!_db_table_name.empty()) {
		DatabaseConnection_ptr icews_db(ICEWSDB::getOutputDb());
		std::string db_table_name = _db_table_name;
		bool oracleDb = icews_db->getSqlVariant() == "Oracle";
		SessionLogger::info("ICEWS") << "SQL variant: " << icews_db->getSqlVariant();
		if (oracleDb) {
			boost::to_upper(db_table_name);
			std::ostringstream existsQuery;
			//std::cerr << "Checking if table exists!" << std::endl;
			existsQuery << "select count(*) from user_tables where "
						<< "table_name = " << icews_db->quote(db_table_name);
			if (icews_db->iter(existsQuery).getCellAsInt32(0) != 0) {
				//std::cerr << "  Exists already -- exiting." << std::endl;
				completed = true;
				return; // Table already exists.
			}
			//std::cerr << "  Does not exist yet -- creating." << std::endl;
		}
		std::ostringstream query;
		query << "CREATE TABLE ";
		if (!oracleDb)
			query << "IF NOT EXISTS " ;
		query << db_table_name << " (";
		size_t decl_field = (oracleDb?ORACLE_DECL_FIELD:SQLITE_DECL_FIELD);
		for (size_t i=0; i<_NUM_COLUMNS; ++i) {
			if (i>0) query << ",";
			if (_output_format == ICEWS_SYM)
				query << CLASSIC_OUTPUT_TABLE_COLUMNS[i][NAME_FIELD] << " " << CLASSIC_OUTPUT_TABLE_COLUMNS[i][decl_field];
			else if (_output_format == WMS_SYM)
				query << WMS_OUTPUT_TABLE_COLUMNS[i][NAME_FIELD] << " " << WMS_OUTPUT_TABLE_COLUMNS[i][decl_field];
			else if (_output_format == CWMD_SYM)
				query << CWMD_OUTPUT_TABLE_COLUMNS[i][NAME_FIELD] << " " << CWMD_OUTPUT_TABLE_COLUMNS[i][decl_field];
			else if (_output_format == SIMPLE_SYM)
				query << SIMPLE_OUTPUT_TABLE_COLUMNS[i][NAME_FIELD] << " " << SIMPLE_OUTPUT_TABLE_COLUMNS[i][decl_field];
		}
		query << ")";
		SessionLogger::info("ICEWS") << "Query for table creation: " << query.str();
		icews_db->exec(query);
		SessionLogger::info("ICEWS") << "Created table " << db_table_name;
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

void ICEWSOutputWriter::saveToSimpleDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory) {
	std::wostringstream query;
	ICEWSEventData eventData = extractEventData(em, docTheory);
	query << L"INSERT INTO " << UnicodeUtil::toUTF16StdString(_db_table_name) << L" ("
		<< L" event_type, document_id, sentence_id, event_tense, "
		<< L" source_actor_id, source_actor_name, source_agent_id, source_agent_name, source_country_codes, "
		<< L" target_actor_id, target_actor_name, target_agent_id, target_agent_name, target_country_codes "
		<< L") VALUES ("
		<< DatabaseConnection::quote(em->getEventType()->getEventCode().to_string()) << ", " 
		<< DatabaseConnection::quote(docTheory->getDocument()->getName().to_string()) << ", "
		<< em->getSentenceTheory()->getSentNumber() << ", " 
		<< DatabaseConnection::quote(em->getEventTense().to_string()) << ", "
		<< eventData.source_actor_id << ", "
		<< DatabaseConnection::quote(eventData.source_actor_name) << ", ";
	if (eventData.source_agent_id == L"")
		query << "NULL,";
	else query << eventData.source_agent_id << ", ";
	query << DatabaseConnection::quote(eventData.source_agent_name) << ", "
		<< DatabaseConnection::quote(eventData.source_country_codes) << ", "
		<< eventData.target_actor_id << ", "
		<< DatabaseConnection::quote(eventData.target_actor_name) << ", ";
	if (eventData.target_agent_id == L"")
		query << "NULL,";
	else query << eventData.target_agent_id << ", ";
	query << DatabaseConnection::quote(eventData.target_agent_name) << ", "
		<< DatabaseConnection::quote(eventData.target_country_codes) << ")";

	DatabaseConnection_ptr icews_db(ICEWSDB::getOutputDb());
	try {
		icews_db->exec(query);
	} catch (UnexpectedInputException &e) {
		// Database might be locked; try again.
		unsigned int delay = 1; // one second
		for (size_t retryNum=0; retryNum<NUM_RETRIES; retryNum++) {
			SessionLogger::warn_user("ICEWS") << "Unable to write to database " << e.getSource() << ": " << e.getMessage() << "; "
										 << "retrying in " << delay << " seconds";
			SessionLogger::warn_user("ICEWS") << "Database write query was: " << query.str() << "\n";	
			std::cout << "Unable to write to database: " << e.getSource() << ": " << e.getMessage() << "\n"
					  << "retrying in " << delay << "seconds" << std::endl;
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


void ICEWSOutputWriter::saveToDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, ActorMention_ptr artifactActor, const DocTheory* docTheory) 
{
	ICEWSEventTypeId eventtype_id = em->getEventType()->getEventId();
	
	// Use the pattern id as the "verbrule" string:
	std::string verbrule = UnicodeUtil::toUTF8StdString(em->getPatternId().to_string());

	// Get the story id.
	StoryId storyId = Stories::extractStoryIdFromDocId(docTheory->getDocument()->getName());
	if (storyId.isNull()) {
		throw UnexpectedInputException("ICEWSOutputWriter::saveToDatabase",
			"Unable to extract story id from docid: ", docTheory->getDocument()->getName().to_debug_string());
	}

	// Determine the sentence number.  Note that SERIF sentences and ICEWS sentences
	// may not always be the same.  Display a warning if any participant falls 
	// outside the ICEWS sentence we identified.
	int serif_sent_no;
	if (sourceActor != ActorMention_ptr())
		serif_sent_no = sourceActor->getEntityMention()->getSentenceNumber();
	else if (artifactActor != ActorMention_ptr())
		serif_sent_no = artifactActor->getEntityMention()->getSentenceNumber();
	std::set<int> participant_sent_number_set;
	typedef std::pair<Symbol, ActorMention_ptr> ParticipantPair;
	typedef std::pair<ParticipantPair, int> ParticipantSentNumPair;
	std::vector<ParticipantSentNumPair> participant_sent_numbers;
	BOOST_FOREACH(const ParticipantPair &participantPair, em->getParticipantList()) {
		const Mention* m = participantPair.second->getEntityMention();
		const SentenceTheory* sentTheory = docTheory->getSentenceTheory(m->getSentenceNumber());
		int tok = m->getAtomicHead()->getStartToken();
		EDTOffset offset = sentTheory->getTokenSequence()->getToken(tok)->getStartEDTOffset();
		int icews_sent_no = IcewsSentenceSpan::edtOffsetToIcewsSentenceNo(offset, docTheory);
		participant_sent_number_set.insert(icews_sent_no);
		participant_sent_numbers.push_back(ParticipantSentNumPair(participantPair, icews_sent_no));
	}
	if (participant_sent_numbers.size() == 0) {
		throw InternalInconsistencyException("ICEWSEventMention::saveToDatabase", 
			"ICEWS event has no participants");
	} else if (participant_sent_number_set.size() > 1) {
		std::ostringstream err;
		err << "ICEWS Event Participants come from different ICEWS sentences:\n";
		BOOST_FOREACH(ParticipantSentNumPair p, participant_sent_numbers) {
			err << "  * [icews_sentno=" << p.second << "] " << p.first.first << " = \"" 
				<< p.first.second->getEntityMention()->toCasedTextString() << "\"\n";
		}
		// SessionLogger TODO
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

	DatabaseConnection_ptr icews_db(ICEWSDB::getOutputDb());

	std::string event_tense(UnicodeUtil::toUTF8StdString(em->getEventTense().to_string()));

	std::ostringstream query;
	if (_output_format == CWMD_SYM || _output_format == ICEWS_SYM) {
		query << "INSERT INTO " << _db_table_name << " ("
			<< " eventtype_id, verbrule, story_id, sentence_num, event_date, coder_id, event_tense";
		addClassicParticipantColumnNamesToQuery("source", query);
		addClassicParticipantColumnNamesToQuery("target", query);
		if (_output_format == CWMD_SYM)
			addClassicParticipantColumnNamesToQuery("artifact", query);
		query << ") VALUES ("
			<< eventtype_id << ", " 
			//<< DatabaseConnection::quote(verbrule) << ", " 
			<< "NULL, " 
			<< storyId << ", "
			<< icews_sent_no << ", " 
			<< icews_db->toDate(event_date) << ", " 
			<< _coder_id << ", " 
			<< DatabaseConnection::quote(event_tense);
		addClassicParticipantIdsToQuery(sourceActor, "source", query);
		addClassicParticipantIdsToQuery(targetActor, "target", query);
		if (_output_format == CWMD_SYM)
			addClassicParticipantIdsToQuery(artifactActor, "artifact", query);
		query << ")";
	}

	try {
		icews_db->exec(query);
	} catch (UnexpectedInputException &e) {
		// Database might be locked; try again.
		unsigned int delay = 1; // one second
		for (size_t retryNum=0; retryNum<NUM_RETRIES; retryNum++) {
			SessionLogger::warn_user("ICEWS") << "Unable to write to database " << e.getSource() << ": " << e.getMessage() << "; "
										 << "retrying in " << delay << " seconds";
			SessionLogger::warn_user("ICEWS") << "Database write query was: " << query.str() << "\n";	
			std::cout << "Unable to write to database: " << e.getSource() << ": " << e.getMessage() << "\n"
					  << "retrying in " << delay << "seconds" << std::endl;
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
		query << DatabaseConnection::quote(_actorInfo->getName(actorID)) << ", ";
		std::vector<ActorId> countries = _actorInfo->getAssociatedCountryActorIds(actorID);
		if (countries.size() > 0) {
			ActorId countryID = countries.at(0);
			query << countryID << ", ";
			query << DatabaseConnection::quote(_actorInfo->getName(countryID)) << ", ";
		} else query << "NULL, NULL, ";
	} else query << "NULL, NULL, NULL, NULL, ";

	if (!agentID.isNull()) {
		query << agentID << ", ";
		query << DatabaseConnection::quote(_actorInfo->getName(agentID));
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

	DatabaseConnection_ptr icews_db(ICEWSDB::getOutputDb());

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

void ICEWSOutputWriter::eventToWMSFormat(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory, std::ostringstream& query)
{
	std::wstring event_code = em->getEventType()->getEventCode().to_string();
	//when testing on data not from WMS, uncomment the following
	std::wstring ss_id(L"abc");
	std::wstring page_id(L"123");
	std::wstring page_version(L"U&Me");

	ActorMention_ptr locationActor = ActorMention_ptr();
	
	//static const boost::wregex DOCID_RE_WMS(L"[a-z]+-[a-z]+-[a-z]+-[a-z]+-[A-Z][A-Z][A-Z][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]_([0-9]+)\\.([0-9]+)\\.([0-9]+)\\..*");
	//boost::wsmatch match;
	//std::wstring docid_str(docTheory->getDocument()->getName().to_string());
	//if (boost::regex_match(docid_str, match, DOCID_RE_WMS)) {
	//	ss_id = match.str(1);
	//	page_id = match.str(2);
	//	page_version = match.str(3);
	//} else {
	//	throw UnexpectedInputException("ICEWSOutputWriter::saveToWMSDatabase",
	//		"Unable to extract ss/page/version information from docid: ", docTheory->getDocument()->getName().to_debug_string());
	//}

	int passage_id = OutputUtil::convertSerifSentenceToPassageId(docTheory, sourceActor->getEntityMention()->getSentenceNumber());
	std::string event_tense(UnicodeUtil::toUTF8StdString(em->getEventTense().to_string()));

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
}
