// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SQLITE_DB_CONNECTION_H
#define SQLITE_DB_CONNECTION_H

#include "Generic/database/DatabaseConnection.h"

// Forward declarations
struct sqlite3;
class QueryProfiler;

/** Implementation of the "DatabaseConnection" interface that
  * is used to connect to sqlite databases. */
class SqliteDBConnection: public DatabaseConnection {
public:
	SqliteDBConnection(const std::string& db_location, bool readonly=true, bool create=false);
	SqliteDBConnection(const std::string& host, int port, const std::string& database, const std::map<std::string,std::string> &parameters);

	virtual ~SqliteDBConnection();

	virtual bool tableExists(const std::wstring& table_name);
	virtual void beginTransaction();
	virtual void endTransaction();
	using DatabaseConnection::exec; // inherit overloads from DatabaseConnection
	virtual Table_ptr exec(const char* query, bool include_column_headers=false);
	using DatabaseConnection::iter; // inherit overloads from DatabaseConnection
	virtual RowIterator iter(const char* query);
	bool readOnly() const;
	void enableProfiling();
	std::string getProfileResults();
	std::string getSqlVariant() { return "Sqlite"; }
	virtual std::string toDate(const std::string &s);
private:
	void connect(const std::string& db_location, bool readonly, bool create);
	struct SqliteRowIteratorCore;
	typedef boost::shared_ptr<SqliteRowIteratorCore> SqliteRowIteratorCore_ptr;
	sqlite3 *_db;
	bool _readOnly;
	QueryProfiler *_queryProfiler;
	std::string _localTempCopy;
};
typedef boost::shared_ptr<SqliteDBConnection> SqliteDBConnection_ptr;

#endif
