// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/InputUtil.h"	
#include "Generic/common/ParamReader.h"
#include "Generic/actors/AWAKEDB.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

/**
 * Singleton implementation used with lazy initialization; entry points are
 * all via static accessor methods.
 **/
AWAKEDB& AWAKEDB::instance() {
	static AWAKEDB instance;
	return instance;
}

/**
 * Calls constructor from ActorDB parent class
 **/
AWAKEDB::AWAKEDB():ActorDB(Symbol(L"bbn_actor_db"),Symbol(L"bbn_actor_db_name"),Symbol(L"bbn_actor_db_list")) {};

/**
 * Get all of the named database connections; this is often used to cache all items
 **/
DatabaseConnectionMap AWAKEDB::getNamedDbs() {
	return instance()._names;
}

/**
 * Looks up the specified database connection by name.
 **/
DatabaseConnection_ptr AWAKEDB::getDb(Symbol db_name) {
	std::stringstream error;
	if (db_name.is_null()) {
		error << "Tried to look up null database name";
		throw UnrecoverableException("AWAKEDB::getDb", error.str().c_str());
	}

	// Looks up the database
	DatabaseConnection_ptr* found_db = instance()._names.get(db_name);
	if (found_db != NULL) {
		return *found_db;
	}

	// Not found
	error << "No database loaded with name '" << db_name << "'";
	throw UnrecoverableException("AWAKEDB::getDb", error.str().c_str());
}

DatabaseConnection_ptr AWAKEDB::getDefaultDb() {
	return instance()._default_db;
}

std::string AWAKEDB::getDefaultDbUrl() {
	return instance()._default_db_url;
}

DatabaseConnection_ptr AWAKEDB::getGeonamesDb() {
	return instance()._geonames_db;
}

Symbol AWAKEDB::getDefaultDbName() {
	return instance()._default_db_name;
}
