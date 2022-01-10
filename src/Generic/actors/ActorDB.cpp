// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/InputUtil.h"	
#include "Generic/common/ParamReader.h"
#include "Generic/actors/ActorDB.h"

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

/**
 * Singleton implementation used with lazy initialization; entry points are
 * all via static accessor methods.
 **/
ActorDB& ActorDB::instance() {
	static ActorDB instance;
	return instance;
}

/**
 * Constructor is protected so that it can be overridden in a subclass
 * but can't be directly accessed.
 **/
ActorDB::ActorDB(Symbol db, Symbol dbname, Symbol dblst) {
	// Read database configuration, handling backwards-compatible single DB mode
	Symbol default_db_url = ParamReader::getParam(db);
	Symbol db_list = ParamReader::getParam(dblst);
	Symbol db_name = ParamReader::getParam(dbname);

	if (!default_db_url.is_null()) {
		// Make configuration mode exclusive to avoid confusion
		if (!db_list.is_null()) {
			throw UnrecoverableException("ActorDB::ActorDB()", "Specify only one db configuration with either <dbname>_db or <dbname>_db_list");
		}
		
		// Connect to a database specified the old way and store it as the default
		_connections[default_db_url] = makeDBConnection(default_db_url.to_debug_string());
		_default_db = _connections[default_db_url];
		_default_db_url = default_db_url.to_debug_string();
		
		// If bbn_actor_db_name param not set, use default db name: otherwise use param
		if (!db_name.is_null()) {
			_names[db_name] = _connections[default_db_url];
			_default_db_name = Symbol(db_name);
		} else {
			_names[default_db_name] = _connections[default_db_url];
			_default_db_name = Symbol(default_db_name);
		}

	} else if (!db_list.is_null()) {
		// Read the list of databases specified in the configuration file
		std::vector< std::vector<std::wstring> > database_configurations = InputUtil::readTabDelimitedFile(db_list.to_debug_string());
		BOOST_FOREACH(std::vector<std::wstring> database_configuration, database_configurations) {
			// Check database configuration row format
			if (database_configuration.size() == 2) {
				Symbol db_name(database_configuration.at(0));
				Symbol db_url(UnicodeUtil::toUTF16StdString(ParamReader::expand(UnicodeUtil::toUTF8StdString(database_configuration.at(1)))));
				_connections[db_url] = makeDBConnection(db_url.to_debug_string());
				_names[db_name] = _connections[db_url];
				if (_connections.size() == 1) {
					// The first entry in the database configuration file should be used as the default
					_default_db = _connections[db_url];
					_default_db_url = db_url.to_debug_string();
					_default_db_name = db_name;
				}
			} else {
				throw UnrecoverableException("ActorDB::ActorDB()", "Invalid database configuration in <dbname>_db_list; need name and url");
			}
		}
	} else {
		throw UnrecoverableException("ActorDB::ActorDB()", "Either parameter '<dbname>_db' or '<dbname>_db_list' must be specified.");
	}
	
	// Connect to other databases, or use a matching existing connection
	_geonames_db = useDBConnection(ParamReader::getParam("gazetteer_db"));
	
}

/**
 * Static helper method that connects to the specified database and
 * optionally enables profiling.
 **/
DatabaseConnection_ptr ActorDB::makeDBConnection(const std::string& url) {
	DatabaseConnection_ptr connected_db = DatabaseConnection::connect(url);
	if (ParamReader::isParamTrue("profile_sql"))
		connected_db->enableProfiling();
	return connected_db;
}

/**
 * Looks up the specified database connection by URL and uses it,
 * or creates it if it doesn't exist. If no URL is specified, the
 * default database is used.
 **/
DatabaseConnection_ptr ActorDB::useDBConnection(const std::string& url) {
	if (url.empty()) {
		return _default_db;
	} else {
		Symbol url_symbol(UnicodeUtil::toUTF16StdString(url));
		DatabaseConnection_ptr db;
		DatabaseConnection_ptr* found_db = _connections.get(url_symbol);
		if (found_db == NULL) {
			db = makeDBConnection(url);
			_connections[url_symbol] = db;
		} else {
			db = *found_db;
		}
		return db;
	}
}
