// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ORACLE_DB_CONNECTION_H
#define ORACLE_DB_CONNECTION_H

#include "Generic/database/DatabaseConnection.h"

/** Implementation of the "DatabaseConnection" interface that
  * is used to connect to Oracle databases. */
class OracleDBConnection: public DatabaseConnection {
public:
	OracleDBConnection(const std::string& host, int port, const std::string& database, const std::map<std::string,std::string> &parameters);
	virtual ~OracleDBConnection();
	virtual bool tableExists(const std::wstring& table_name);
	virtual void beginTransaction();
	virtual void endTransaction();
	using DatabaseConnection::exec;
	virtual Table_ptr exec(const char* query, bool include_column_headers=false);
	using DatabaseConnection::iter;
	virtual RowIterator iter(const char* query);
	bool readOnly() const { return false; }
	void enableProfiling();
	std::string getProfileResults();
	std::string getSqlVariant() { return "Oracle"; }
	virtual std::string toDate(const std::string &s);
protected:
	virtual std::string rowLimitQuery(const std::string& query, size_t block_start, size_t block_size);
private:
	struct OracleRowIteratorCore;
	typedef boost::shared_ptr<OracleRowIteratorCore> OracleRowIteratorCore_ptr;
	struct Impl;
	Impl *_impl;
};

#endif
