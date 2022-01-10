// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef MYSQL_DB_CONNECTION_H
#define MYSQL_DB_CONNECTION_H

#include "Generic/database/DatabaseConnection.h"

/** Implementation of the "DatabaseConnection" interface that
  * is used to connect to MySQL databases. */
class MySQLDBConnection: public DatabaseConnection {
public:
	MySQLDBConnection(const std::string& host, int port, const std::string& database, const std::map<std::string,std::string> &parameters);
	virtual ~MySQLDBConnection();
	virtual bool tableExists(const std::wstring& table_name);
	virtual void beginTransaction();
	virtual void endTransaction();
	using DatabaseConnection::exec;
	virtual Table_ptr exec(const char* query, bool include_column_headers=false);
	using DatabaseConnection::iter;
	virtual RowIterator iter(const char* query);
	bool readOnly() const { return false; }
	void enableProfiling();
	std::string getSqlVariant() { return "MySQL"; }
	virtual std::string toDate(const std::string &s);
private:
	struct MySQLRowIteratorCore;
	typedef boost::shared_ptr<MySQLRowIteratorCore> MySQLRowIteratorCore_ptr;
	struct Impl;
	Impl *_impl;
};

#endif
