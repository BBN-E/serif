// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BBN_ACTOR_DB_H
#define BBN_ACTOR_DB_H

#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorDB.h"

typedef Symbol::HashMap<DatabaseConnection_ptr> DatabaseConnectionMap;
typedef Symbol::HashMap<Symbol> SymbolSymbolMap;

class AWAKEDB;
typedef boost::shared_ptr<AWAKEDB> AWAKEDB_ptr;

/**
 * A multiton implementation for accessing AWAKE databases; there will be one
 * instance of this class, containing multiple database connections organized
 * by URL and other configured metadata.
 **/
class AWAKEDB : public ActorDB {
public:
	static DatabaseConnectionMap getNamedDbs();
	static DatabaseConnection_ptr getDb(Symbol db_name);
	static DatabaseConnection_ptr getDefaultDb();
	static Symbol getDefaultDbName();
	static std::string getDefaultDbUrl();
	static DatabaseConnection_ptr getGeonamesDb();

	template<typename Tag, typename IdType>
	static DatabaseConnection_ptr getDb(const ICEWSIdentifier<Tag, IdType>& awake_id) { if (awake_id.isNull()) { return getDefaultDb(); } else { return getDb(awake_id.getDbName()); } }

protected:
	AWAKEDB();
	~AWAKEDB() { }
	static AWAKEDB& instance();

};

#endif

