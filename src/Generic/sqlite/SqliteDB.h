// Copyright (c) 2009 by BBNT Solutions LLC
// All Rights Reserved.

/** This header is provided for backwards compatiblity; please use
  * Generic/database/SqliteDBConnection.h instead. */

#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include "Generic/database/SqliteDBConnection.h"
class SqliteDB: public SqliteDBConnection {
public:
	SqliteDB(const std::string& db_location, bool readonly=true, bool create=false)
		: SqliteDBConnection(db_location, readonly, create) {}
};
typedef boost::shared_ptr<SqliteDB> SqliteDB_ptr;
typedef DatabaseConnection::Table_ptr Table_ptr;
typedef DatabaseConnection::Table Table;
typedef DatabaseConnection::TableRow TableRow;

#endif
