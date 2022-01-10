// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_OUTPUT_WRITER_H
#define ICEWS_OUTPUT_WRITER_H

#include "Generic/icews/ICEWSEventMention.h"
#include "Generic/driver/DocumentDriver.h"
#include "Generic/actors/ActorInfo.h"
#include <boost/noncopyable.hpp>
#include <string>

// Forward declarations
class DocTheory;

/** A document-level processing class used save the ICEWS results to
  * a database table. */
class ICEWSOutputWriter: public DocumentDriver::DocTheoryStageHandler, private boost::noncopyable {
public:
	ICEWSOutputWriter();
	~ICEWSOutputWriter();

	struct ICEWSDocData {
		std::wstring wms_ss_id;
		std::wstring wms_page_id;
	};

	// ICEWSEventData struct holds all pertinent event data for a single event
	struct ICEWSEventData {
		std::wstring wms_passage_id; 
		std::wstring event_code; //int
		std::wstring event_tense;
		std::wstring source_actor_id; //int
		std::wstring source_actor_name;
		std::wstring source_agent_id; //int
		std::wstring source_agent_name;
		std::wstring source_country_code;
		std::wstring source_country_codes; // allows more than one
		std::wstring target_actor_id; //int
		std::wstring target_actor_name;
		std::wstring target_agent_id; //int
		std::wstring target_agent_name;
		std::wstring target_country_code;
		std::wstring target_country_codes; // allows more than one
		std::wstring location_name;
		std::wstring location_country_code;
		std::wstring location_latitude; //float
		std::wstring location_longitude; //float
		std::wstring event_date;
	};

	/** Save all ICEWSEventMentions in the doctheory to the ICEWS 
	  * knowledge database. */
	void process(DocTheory *dt);

	/* Save ICEWSEventMentions in the doctheory to an XML file to be used for uploading event
	data to the WMS*/
	void processForWMS(DocTheory *dt);

private:
	Symbol _output_format;
	size_t _NUM_COLUMNS;
	std::string _db_table_name;
	bool _check_for_duplicates;
	ActorInfo_ptr _actorInfo;

	void saveToDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory);
	void ensureOutputTableExists();
	
	void saveToSimpleDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory);
	void saveToDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, ActorMention_ptr artifactActor, const DocTheory* docTheory);
	void addClassicParticipantColumnNamesToQuery(const char* role, std::ostringstream &query);
	void addClassicParticipantIdsToQuery(ActorMention_ptr actor, const char* role, std::ostringstream &query);

	// extractDocumentData parses WMS unique identifiers from the document id
	ICEWSDocData extractDocumentData(const DocTheory*);
	/* docDataToXML, eventToXML take the ICEWSDocData / ICEWSEventData structs defined above and generate
	XML snippets from them. This xml is used to upload events to WMS */
	std::wstring docDataToXML(ICEWSDocData docData, std::wstring indent=L"  ");
	std::wstring eventToXML(ICEWSEventData eventData, std::wstring indent=L"\t");
	// extractEventData populates an ICEWSEventData struct with event information to be exported to xml
	ICEWSEventData extractEventData(ICEWSEventMention_ptr em, const DocTheory* docTheory);
	
	// legacy functions used to populate an icews event mention database 
	void eventToWMSFormat(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory, std::ostringstream& query);
	void saveToWMSDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory);
	void addWMSLocationToQuery(ActorMention_ptr actor, std::ostringstream &query);
	void addWMSParticipantToQuery(ActorMention_ptr actor, std::ostringstream &query);

	int _coder_id;

	/* helper functions for extracting event data */

	// add document id related data (ss_id, page_id, etc.) to ICEWSEventData struct
	void extractDocumentData(const DocTheory*, ICEWSEventData& eventData);
	// add event location data (latitude, longitude, etc.) to ICEWSEventData struct
	void extractEventLocationData(ProperNounActorMention_ptr locationActor, ICEWSEventData& eventData);
	// add event source actor data (actor_id, actor_name, etc.) to ICEWSEventData struct
	void extractEventSourceData(ActorMention_ptr sourceActor, ICEWSEventData& eventData);
	// add event target actor data (actor_id, actor_name, etc.) to ICEWSEventData struct 
	void extractEventTargetData(ActorMention_ptr targetActor, ICEWSEventData& eventData);
};

#endif
