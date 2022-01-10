// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ICEWSDB_H
#define ICEWSDB_H

#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/Symbol.h"
#include "Generic/actors/Identifiers.h"
#include "Generic/actors/ActorDB.h"

typedef Symbol::HashMap<DatabaseConnection_ptr> DatabaseConnectionMap;
typedef Symbol::HashMap<Symbol> SymbolSymbolMap;

class ICEWSDB;
typedef boost::shared_ptr<ICEWSDB> ICEWSDB_ptr;

/**
 * A multiton implementation for accessing ICEWS databases; there will be one
 * instance of this class, containing multiple database connections organized
 * by URL and other configured metadata.
 **/
class ICEWSDB : public ActorDB {
public:
	static DatabaseConnectionMap getNamedDbs();
	static DatabaseConnection_ptr getDb(Symbol db_name);
	static DatabaseConnection_ptr getDefaultDb();
	static std::string getDefaultDbUrl();
	static Symbol getDefaultDbName();
	static DatabaseConnection_ptr getStoriesDb();
	static DatabaseConnection_ptr getStorySerifXMLDb();
	static DatabaseConnection_ptr getOutputDb();
	static DatabaseConnection_ptr getGeonamesDb();

	template<typename Tag, typename IdType>
	static DatabaseConnection_ptr getDb(const ICEWSIdentifier<Tag, IdType>& icews_id) { if (icews_id.isNull()) { return getDefaultDb(); } else { return getDb(icews_id.getDbName()); } }
	
protected:
	ICEWSDB();
	~ICEWSDB() { }
	static ICEWSDB& instance();

private:
	DatabaseConnection_ptr _stories_db;
	DatabaseConnection_ptr _story_serifxml_db;
	DatabaseConnection_ptr _output_db;

};

#endif
