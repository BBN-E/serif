// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef POSTGRES_DB_CONNECTION_H
#define POSTGRES_DB_CONNECTION_H

#include "Generic/database/DatabaseConnection.h"
//for libpqxx
#include <pqxx/pqxx>
using namespace pqxx;

// Forward declarations
class QueryProfiler;

/** Implementation of the "DatabaseConnection" interface that
  * is used to connect to PostgreSQL databases. */
class PostgresDBConnection: public DatabaseConnection {
public:
	PostgresDBConnection(const std::string& host, int port, const std::string& database, 
		const std::string& username, const std::string& password);
	
	PostgresDBConnection(const std::string& host, int port, const std::string& database, const std::map<std::string,std::string> &parameters);

	virtual ~PostgresDBConnection();

	virtual bool tableExists(const std::wstring& table_name);
	virtual void beginTransaction();
	virtual void endTransaction();
	bool readOnly() const;
	void enableProfiling();
	std::string getProfileResults();
	using DatabaseConnection::exec;
	virtual Table_ptr exec(const char* query, bool include_column_headers=false);
	using DatabaseConnection::iter; // inherit overloads from DatabaseConnection
	virtual RowIterator iter(const char* query);
	std::string getSqlVariant() { return "postgresql"; }
	virtual std::string toDate(const std::string &s);
	virtual std::string escape_and_quote(const std::string &s);
private:
	struct PostgresRowIteratorCore;
	typedef boost::shared_ptr<PostgresRowIteratorCore> PostgresRowIteratorCore_ptr;
	QueryProfiler *_queryProfiler;
	//libpqxx stuff
	connection* conn;

};
typedef boost::shared_ptr<PostgresDBConnection> PostgresDBConnection_ptr;

#endif
