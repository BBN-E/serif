// Copyright 2017 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_DB_H
#define ACTOR_DB_H

#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"

typedef Symbol::HashMap<DatabaseConnection_ptr> DatabaseConnectionMap;
typedef Symbol::HashMap<Symbol> SymbolSymbolMap;

class ActorDB;
typedef boost::shared_ptr<ActorDB> ActorDB_ptr;

class ActorDB {
public:
	ActorDB() {};
	ActorDB(Symbol db,Symbol dbname, Symbol dblst);
	~ActorDB() { }
	static ActorDB& instance();

	static DatabaseConnection_ptr makeDBConnection(const std::string& url);
	DatabaseConnection_ptr useDBConnection(const std::string& url);

	// Connections are stored by their URL
	DatabaseConnectionMap _connections;

	// We also store some connections by their named shortcut
	DatabaseConnectionMap _names;

	// Default database connection references the first loaded database in
	// the connections map
	DatabaseConnection_ptr _default_db;
	std::string _default_db_url;
	Symbol _default_db_name;

	// Special database connection member that references into the connections map
	DatabaseConnection_ptr _geonames_db;

};

#endif
