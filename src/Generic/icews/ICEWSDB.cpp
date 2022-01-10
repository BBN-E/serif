// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/InputUtil.h"	
#include "Generic/common/ParamReader.h"
#include "Generic/icews/ICEWSDB.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

/**
 * Singleton implementation used with lazy initialization; entry points are
 * all via static accessor methods.
 **/
ICEWSDB& ICEWSDB::instance() {
	static ICEWSDB instance;
	return instance;
}

/**
 * Call constructor from ActorDB parent class
 **/
ICEWSDB::ICEWSDB():ActorDB(Symbol(L"icews_db"),Symbol(L"icews_db_name"),Symbol(L"icews_db_list")) {
    // Connect to other databases, or use a matching existing connection
	_stories_db = useDBConnection(ParamReader::getParam("icews_stories_db"));
	_story_serifxml_db = useDBConnection(ParamReader::getParam("icews_story_serifxml_db"));
	_output_db = useDBConnection(ParamReader::getParam("icews_output_db"));
};

/**
 * Get all of the named database connections; this is often used to cache all items
 **/
DatabaseConnectionMap ICEWSDB::getNamedDbs() {
	return instance()._names;
}

/**
 * Looks up the specified database connection by name.
 **/
DatabaseConnection_ptr ICEWSDB::getDb(Symbol db_name) {
	std::stringstream error;
	if (db_name.is_null()) {
		error << "Tried to look up null database name";
		throw UnrecoverableException("ICEWSDB::getDb", error.str().c_str());
	}

	// Looks up the database
	DatabaseConnection_ptr* found_db = instance()._names.get(db_name);
	if (found_db != NULL) {
		return *found_db;
	}

	// Not found
	error << "No database loaded with name '" << db_name << "'";
	throw UnrecoverableException("ICEWSDB::getDb", error.str().c_str());
}

DatabaseConnection_ptr ICEWSDB::getDefaultDb() {
	return instance()._default_db;
}

std::string ICEWSDB::getDefaultDbUrl() {
	return instance()._default_db_url;
}

DatabaseConnection_ptr ICEWSDB::getStoriesDb() {
	return instance()._stories_db;
}

DatabaseConnection_ptr ICEWSDB::getStorySerifXMLDb() {
	return instance()._story_serifxml_db;
}

DatabaseConnection_ptr ICEWSDB::getOutputDb() {
	return instance()._output_db;
}


DatabaseConnection_ptr ICEWSDB::getGeonamesDb() {
	return instance()._geonames_db;
}

Symbol ICEWSDB::getDefaultDbName() {
	return instance()._default_db_name;
}
