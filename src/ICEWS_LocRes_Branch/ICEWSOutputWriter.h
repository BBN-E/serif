// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWS_OUTPUT_WRITER_H
#define ICEWS_OUTPUT_WRITER_H

#include "ICEWS/EventMention.h"
#include "Generic/driver/DocumentDriver.h"
#include <boost/noncopyable.hpp>
#include <string>

// Forward declarations
class DocTheory;

namespace ICEWS {

/** A document-level processing class used save the ICEWS results to
  * a database table. */
class ICEWSOutputWriter: public DocumentDriver::DocTheoryStageHandler, private boost::noncopyable {
public:
	ICEWSOutputWriter();
	~ICEWSOutputWriter();

	/** Save all ICEWSEventMentions in the doctheory to the ICEWS 
	  * knowledge database. */
	void process(DocTheory *dt);

private:
	typedef enum { CLASSIC_ICEWS, WMS } output_type_t;
	output_type_t _output_type;
	size_t _NUM_COLUMNS;
	std::string _db_table_name;
	bool _check_for_duplicates;
	void saveToDatabase(ICEWSEventMention_ptr em, const DocTheory* docTheory);
	void ensureOutputTableExists();
	
	void saveToClassicDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory);
	void addClassicParticipantColumnNamesToQuery(const char* role, std::ostringstream &query);
	void addClassicParticipantIdsToQuery(ActorMention_ptr actor, const char* role, std::ostringstream &query);

	void saveToWMSDatabase(ICEWSEventMention_ptr em, ActorMention_ptr sourceActor, ActorMention_ptr targetActor, const DocTheory* docTheory);
	void addWMSLocationToQuery(ActorMention_ptr actor, std::ostringstream &query);
	void addWMSParticipantToQuery(ActorMention_ptr actor, std::ostringstream &query);

	int _coder_id;

};

} // end ICEWS namespace

#endif
