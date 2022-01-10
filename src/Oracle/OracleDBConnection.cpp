// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/database/QueryProfiler.h"
#include "Oracle/OracleDBConnection.h"
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/regex.hpp>

#ifdef WIN32
  #include "winsock.h"
  #include "occi.h"
  #include <Windows.h>
  #define sleep(x) Sleep((x)*1000)
#else
  #include "occi.h"
  #include <unistd.h>
#endif

namespace {
	void throwOracleError(sword errcode, std::string errmsg=std::string(), const char* query=0) {
		std::ostringstream err;
		err << "Oracle error " << errcode;
		if (!errmsg.empty())
			err << ": "+errmsg;
		if (query)
			err << "\nQUERY:\n" << query;
		throw UnexpectedInputException("OracleDBConnection", err.str().c_str());
	}
	bool isStringTrue(const std::string &s) {
		return (boost::iequals(s, "true") || boost::iequals(s, "on") ||
		        boost::iequals(s, "yes") || boost::iequals(s, "1"));
	}

}

struct OracleDBConnection::Impl: private boost::noncopyable {
	// Information about where we should connect.
	struct OracleConnectionInfo: private boost::noncopyable {
		QueryProfiler *queryProfiler;
		std::string hostname;
		std::string username;
		std::string password;
		std::string database;
		size_t socket;
		bool keep_open; // if false, then re-open connection for each query.
		int open_retries;
		unsigned int connect_timeout;

		OracleConnectionInfo(const std::string& host, int port, const std::string& database, 
		                    const std::map<std::string,std::string> &parameters)
							: hostname(host), socket(port), database(database), username("user"), 
							password("pass"), keep_open(true), open_retries(0), connect_timeout(0)
		{
			typedef std::pair<std::string,std::string> StringPair;
			BOOST_FOREACH(const StringPair &param, parameters) {
				if (boost::iequals(param.first, "user") ||
					boost::iequals(param.first, "username"))
					username = param.second;
				else if (boost::iequals(param.first, "pass") ||
					boost::iequals(param.first, "password"))
					password = param.second;
				else if (boost::iequals(param.first, "keep_open"))
					keep_open = isStringTrue(param.second);
				else if (boost::iequals(param.first, "open_retries"))
					open_retries = boost::lexical_cast<int>(param.second);
				else if (boost::iequals(param.first, "connection_timeout"))
					connect_timeout = boost::lexical_cast<int>(param.second);
			}
		}
	};

	struct ActiveOracleConnection: private boost::noncopyable {
		oracle::occi::Environment *env;
		oracle::occi::Connection *conn;

		// Todo: try/catch error handling??
		// Todo: timeout/retry?
		ActiveOracleConnection(const OracleConnectionInfo &info): env(0), conn(0) {
			env = oracle::occi::Environment::createEnvironment("AL32UTF8", "AL32UTF8");
			std::ostringstream connectId;
			if (boost::iequals(info.hostname, "tnsname")) {
				if (info.socket != 0)
					throw UnexpectedInputException("OracleDBConnection", 
												   "socket may not be specified when using oracle://tnsname/xyz");
				connectId << info.database;
			} else {
				connectId << "//" << info.hostname << ":" << info.socket << "/" << info.database;
			}
			SessionLogger::info("SQL") 
				<< "Using Oracle connectId: [" << connectId.str() << "]";
			conn = env->createConnection(info.username, info.password, connectId.str());
		}

		~ActiveOracleConnection() {
			if (env) {
				if (conn) {
					env->terminateConnection(conn);
					conn = 0;
				}
				oracle::occi::Environment::terminateEnvironment(env);
				env = 0;
			}
		}
	};
	typedef boost::shared_ptr<ActiveOracleConnection> ActiveOracleConnection_ptr;

public:
	OracleConnectionInfo info;
	QueryProfiler *queryProfiler;

	ActiveOracleConnection_ptr getHandle() {
		if (shared_handle) 
			return shared_handle;
		else 
			return boost::make_shared<ActiveOracleConnection>(info);
	}

	Impl(const std::string& host, int port, const std::string& database, 
	     const std::map<std::string,std::string> &parameters)
		 : info(host, port, database, parameters), queryProfiler(0) 
	{
		if (info.keep_open)
			shared_handle = boost::make_shared<ActiveOracleConnection>(info);
	}
private:
	boost::shared_ptr<ActiveOracleConnection> shared_handle; // only used if keep_open=true.
};

namespace {
	static const boost::regex SQL_UPDATE_RE("^(INSERT|UPDATE|DELETE)\\s.*");

}

OracleDBConnection::OracleDBConnection(const std::string& host, int port, const std::string& database, 
									 const std::map<std::string,std::string> &parameters)
									 : _impl(_new OracleDBConnection::Impl(host, port, database, parameters)) {}

OracleDBConnection::~OracleDBConnection() {
	if (_impl) {
		if (_impl->queryProfiler) {
			SessionLogger::info("SQL") << _impl->queryProfiler->getResults();
			delete _impl->queryProfiler;
		}
		delete _impl;
		_impl = 0;
	}
}

void OracleDBConnection::enableProfiling() {
	if (!_impl->queryProfiler)
		_impl->queryProfiler = _new QueryProfiler();
}

std::string OracleDBConnection::getProfileResults() {
	if (_impl->queryProfiler) {
		return _impl->queryProfiler->getResults();
	} 
	return "";
}

bool OracleDBConnection::tableExists(const std::wstring& table_name) {
	std::ostringstream query;
	query << "SELECT 1 FROM information_schema.tables WHERE table_name = "
		<< quote(table_name);
	return iter(query) != end();
}

void OracleDBConnection::beginTransaction() {
	iter("BEGIN");
}
void OracleDBConnection::endTransaction() {
	iter("COMMIT");
}

DatabaseConnection::Table_ptr OracleDBConnection::exec(const char* query, bool include_column_headers) {
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_impl->queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_impl->queryProfiler->timerFor(query));
	if (queryTimer) queryTimer->start();

	// If keep_open=false, then this will handle connecting & disconnecting:
	OracleDBConnection::Impl::ActiveOracleConnection_ptr handle = _impl->getHandle();

	oracle::occi::Statement *stmt = handle->conn->createStatement(query);
	oracle::occi::ResultSet *result = 0;
	try {
		if (boost::regex_match(query, SQL_UPDATE_RE)) {
			//std::cout << "EXECUTING UPDATE " << query << std::endl;
			stmt->executeUpdate();
			handle->conn->commit();
		} else {
			result = stmt->executeQuery();
		}
	} catch (oracle::occi::SQLException ex) {
		handle->conn->terminateStatement(stmt);
		throwOracleError(ex.getErrorCode(), ex.getMessage(), query);
	}
	handle->conn->terminateStatement(stmt);

	if (!result)
		return boost::make_shared<Table>(); // for non-select queries.

	unsigned int num_columns = result->getColumnListMetaData().size();
	Table_ptr table = boost::make_shared<Table>();

	// Get the column headers, if requested.
	if (include_column_headers) {
		(*table).resize(table->size()+1);
		TableRow &row = table->back();
		BOOST_FOREACH(oracle::occi::MetaData meta, result->getColumnListMetaData()) {
			row.push_back(UnicodeUtil::toUTF16StdString(meta.getString(oracle::occi::MetaData::ATTR_NAME)));
		}
	}

	// Get the data rows
	while (result->next()) {
		TableRow &tableRow = table->back();
		for (unsigned int i=0; i<num_columns; ++i) {
			tableRow.push_back(UnicodeUtil::toUTF16StdString(result->getString(i+1)));
		}
	}

	if (queryTimer) queryTimer->stop();
	return table;
}

struct OracleDBConnection::OracleRowIteratorCore: public DatabaseConnection::RowIteratorCore {
	OracleRowIteratorCore(OracleDBConnection::Impl *impl, const char* query)
		: result(0), num_columns(0), queryTimer(0), handle(impl->getHandle()), stmt(0)
	{
		if (impl->queryProfiler) {
			queryTimer = _new QueryProfiler::QueryTimer(impl->queryProfiler->timerFor(query));
			queryTimer->start();
		}

		stmt = handle->conn->createStatement(query);
		try {
			if (boost::regex_match(query, SQL_UPDATE_RE)) {
				stmt->executeUpdate();
			} else {
				result = stmt->executeQuery();
			}
		} catch (oracle::occi::SQLException ex) {
			handle->conn->terminateStatement(stmt);
			throwOracleError(ex.getErrorCode(), ex.getMessage(), query);
		}

		if (!result) return; // EOF iterator: no result (eg CREATE TABLE command)
		std::vector<oracle::occi::MetaData> metadata = result->getColumnListMetaData();
		num_columns = metadata.size();
		for (size_t i=0; i<num_columns; ++i)  {
			colTypes.push_back(metadata[i].getInt(oracle::occi::MetaData::ATTR_DATA_TYPE));
		}
		if (!result->next()) {
			stmt->closeResultSet(result);
			handle->conn->terminateStatement(stmt);
			result = 0; // EOF iterator: empty table
			stmt = 0;
		}
	}

	virtual ~OracleRowIteratorCore() { 
		if (result) stmt->closeResultSet(result);
		if (stmt) handle->conn->terminateStatement(stmt);
		result = 0;
		stmt = 0;
		if (queryTimer) {
			queryTimer->stop();
			delete queryTimer;
		}
	}

	// Note: return value gets invalidated next time getCell is called!
	virtual const char* getCell(size_t column) {
		if (isEOF())
			throw UnexpectedInputException("OracleDBConnection::OracleRowIteratorCore::getCell",
				"Attempt to get cell from EOF iterator");
		if (column >= num_columns)
			throw UnexpectedInputException("OracleDBConnection::OracleRowIteratorCore::getCell",
				"Column number out of bounds");
		if (result->isNull(column+1))
			return 0;
		else {
			if (colTypes[column] == oracle::occi::OCCI_SQLT_CLOB) {
				oracle::occi::Clob clob = result->getClob(column+1 ); 
				if (!clob.isNull()) {
					oracle::occi::Stream *instream = clob.getStream (1,0);
					const int size = clob.length()*4; // UTF-8 worst-case
					cell.clear();
					cell.resize(size, 0);
					instream->readBuffer (&cell[0], size);
					clob.closeStream (instream);
				}
			} else {
				cell = result->getString(column+1);
			}

			return cell.c_str();
		}
	}

	virtual size_t getNumColumns() {
		return num_columns;
	}

	virtual void fetchNextRow() {
		if (result == NULL)
			throw UnexpectedInputException("OracleDBConnection::OracleRowIteratorCore::fetchNextRow",
				"Already at end-of-table.");
		if (!result->next()) {
			stmt->closeResultSet(result);
			handle->conn->terminateStatement(stmt);
			result = 0; // EOF iterator
			stmt = 0;
		}
	}

	virtual bool isEOF() const { 
		return (result == 0) || (stmt == 0); 
	}

private:
	OracleDBConnection::Impl::ActiveOracleConnection_ptr handle;
	// Result:
	oracle::occi::Statement *stmt;
	oracle::occi::ResultSet *result;
	unsigned int num_columns;
	std::vector<int> colTypes;
	QueryProfiler::QueryTimer *queryTimer;
	std::string cell;
};

DatabaseConnection::RowIterator OracleDBConnection::iter(const char* query) {
	return RowIterator(boost::shared_ptr<RowIteratorCore>(_new OracleRowIteratorCore(_impl, query)));
}

std::string OracleDBConnection::toDate(const std::string &s) {
	static boost::regex YYYY_MM_DD("^([0-9][0-9][0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9])");
	static boost::regex MM_DD_YYYY("^([0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9][0-9][0-9])");
	static boost::regex DD_MON_YY("^([0-9][0-9])[\\-/]([A-Za-z][A-Za-z][A-Za-z])[\\-/]([0-9][0-9])");
	std::ostringstream out;

	if (boost::regex_match(s, YYYY_MM_DD))
		out << "TO_DATE(" << quote(s) << ",'YYYY-MM-DD')";
	else if (boost::regex_match(s, MM_DD_YYYY))
		out << "TO_DATE(" << quote(s) << ",'MM-DD-YYYY')";
	else if (boost::regex_match(s, DD_MON_YY))
		out << "TO_DATE(" << quote(s) << ",'DD-MON-YY')";
	else {
		SessionLogger::err("ICEWS") << "quoteDate: unexpected date format: " << s;
		out << "null";
	}
	return out.str();
}

std::string OracleDBConnection::rowLimitQuery(const std::string& query, size_t block_start, size_t block_size) {
	std::ostringstream limited_query;
	limited_query << "select * from "
				  << "( select a.*, ROWNUM rnum from "
				  << "( " << query << " ) a "
				  << "where ROWNUM < " << (1+block_start+block_size) << " ) "
				  << "where rnum  >= " << (1+block_start);
	//std::cout << "DEBUG: " << limited_query.str() << std::endl;
	//SessionLogger::err("ICEWS") << "DEBUG: " << limited_query.str();
	return limited_query.str();
}
