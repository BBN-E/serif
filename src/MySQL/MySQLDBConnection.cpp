// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/database/QueryProfiler.h"
#include "MySQL/MySQLDBConnection.h"
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/regex.hpp>

#ifdef WIN32
  #include "winsock.h"
  #include "mysql.h"
  #include <Windows.h>
  #define sleep(x) Sleep((x)*1000)
#else
  #include "mysql.h"
  #include <unistd.h>
#endif

namespace {
	void throwMySQLError(MYSQL* conn, const char* query=0) {
		std::ostringstream err;
		err << "MySQL error " << mysql_errno(conn) << ": " <<  mysql_error(conn);
		if (query)
			err << "\nQUERY:\n" << query;
		throw UnexpectedInputException("MySQLDBConnection", err.str().c_str());
	}
	bool isStringTrue(const std::string &s) {
		return (boost::iequals(s, "true") || boost::iequals(s, "on") ||
		        boost::iequals(s, "yes") || boost::iequals(s, "1"));
	}

}

struct MySQLDBConnection::Impl: private boost::noncopyable {
	// Information about where we should connect.
	struct MySQLConnectionInfo: private boost::noncopyable {
		QueryProfiler *queryProfiler;
		std::string hostname;
		std::string username;
		std::string password;
		std::string database;
		size_t socket;
		bool keep_open; // if false, then re-open connection for each query.
		int open_retries;
		unsigned int connect_timeout;

		MySQLConnectionInfo(const std::string& host, int port, const std::string& database, 
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

	struct ActiveMySQLConnection: private boost::noncopyable {
		MYSQL *conn;

		ActiveMySQLConnection(const MySQLConnectionInfo &info): conn(0) {
			conn = mysql_init(NULL);

			if (conn==NULL)
				throwMySQLError(conn);

			// Allow old password format
			my_bool secure_auth = false;
			mysql_options(conn, MYSQL_SECURE_AUTH, &secure_auth); 
			
			if (mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8"))
				throwMySQLError(conn);
			if (info.connect_timeout != 0)
				if (mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &info.connect_timeout))
					throwMySQLError(conn);
			if (mysql_options(conn, MYSQL_INIT_COMMAND , "SET SESSION sql_mode = 'NO_BACKSLASH_ESCAPES'"))
				throwMySQLError(conn);
			if (mysql_real_connect(conn, info.hostname.c_str(), info.username.c_str(), info.password.c_str(), info.database.c_str(), info.socket, NULL, 0)==NULL) {
				unsigned int delay = 1; // one second.
				bool connected = false;
				for (int i=0; i<info.open_retries && !connected; ++i) {
					std::cerr << "Unable to connect to \"mysql://" << info.hostname;
					if (info.socket)
						std::cerr << ":" << info.socket;
					std::cerr << "/" << info.database << "\"; retrying in " << delay << " seconds." << std::endl;
					sleep(delay);
					delay *= 2;
					if (mysql_real_connect(conn, info.hostname.c_str(), info.username.c_str(), info.password.c_str(), info.database.c_str(), info.socket, NULL, 0)!=NULL)
						connected = true;
				}
				if (!connected)
					throwMySQLError(conn);
			}
		}
		~ActiveMySQLConnection() {
			mysql_close(conn);
			conn = 0;
		}
	};
	typedef boost::shared_ptr<ActiveMySQLConnection> ActiveMySQLConnection_ptr;

public:
	MySQLConnectionInfo info;
	QueryProfiler *queryProfiler;

	ActiveMySQLConnection_ptr getHandle() {
		if (shared_handle) 
			return shared_handle;
		else 
			return boost::make_shared<ActiveMySQLConnection>(info);
	}

	Impl(const std::string& host, int port, const std::string& database, 
	     const std::map<std::string,std::string> &parameters)
		 : info(host, port, database, parameters), queryProfiler(0) 
	{
		if (info.keep_open)
			shared_handle = boost::make_shared<ActiveMySQLConnection>(info);
	}
private:
	boost::shared_ptr<ActiveMySQLConnection> shared_handle; // only used if keep_open=true.
};

MySQLDBConnection::MySQLDBConnection(const std::string& host, int port, const std::string& database, 
									 const std::map<std::string,std::string> &parameters)
									 : _impl(_new MySQLDBConnection::Impl(host, port, database, parameters)) {}

MySQLDBConnection::~MySQLDBConnection() {
	if (_impl) {
		if (_impl->queryProfiler) {
			SessionLogger::info("SQL") << _impl->queryProfiler->getResults();
			delete _impl->queryProfiler;
		}
		delete _impl;
		_impl = 0;
	}
}

void MySQLDBConnection::enableProfiling() {
	if (!_impl->queryProfiler)
		_impl->queryProfiler = _new QueryProfiler();
}

bool MySQLDBConnection::tableExists(const std::wstring& table_name) {
	std::ostringstream query;
	query << "SELECT 1 FROM information_schema.tables WHERE table_name = "
		<< quote(table_name);
	return iter(query) != end();
}

void MySQLDBConnection::beginTransaction() {
	iter("BEGIN");
}
void MySQLDBConnection::endTransaction() {
	iter("COMMIT");
}

std::string MySQLDBConnection::toDate(const std::string &s) {
	static boost::regex YYYY_MM_DD("^([0-9][0-9][0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9])");
	static boost::regex MM_DD_YYYY("^([0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9][0-9][0-9])");
	static boost::regex DD_MON_YY("^([0-9][0-9])[\\-/]([A-Za-z][A-Za-z][A-Za-z])[\\-/]([0-9][0-9])");
	std::ostringstream out;

	if (boost::regex_match(s, YYYY_MM_DD))
		out << "STR_TO_DATE(" << quote(s) << ",'%Y-%m-%d')";
	else if (boost::regex_match(s, MM_DD_YYYY))
		out << "STR_TO_DATE(" << quote(s) << ",'%m-%d-%Y')";
	else if (boost::regex_match(s, DD_MON_YY))
		out << "STR_TO_DATE(" << quote(s) << ",'%d-%b-%y')";
	else {
		SessionLogger::err("ICEWS") << "quoteDate: unexpected date format: " << s;
		out << "null";
	}
	return out.str();
}

DatabaseConnection::Table_ptr MySQLDBConnection::exec(const char* query, bool include_column_headers) {
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_impl->queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_impl->queryProfiler->timerFor(query));
	if (queryTimer) queryTimer->start();

	// If keep_open=false, then this will handle connecting & disconnecting:
	MySQLDBConnection::Impl::ActiveMySQLConnection_ptr handle = _impl->getHandle();

	if (mysql_query(handle->conn, query))
		throwMySQLError(handle->conn, query);

	MYSQL_RES *result = mysql_store_result(handle->conn);
	if (!result)
		return boost::make_shared<Table>(); // for non-select queries.
	my_ulonglong num_rows = mysql_num_rows(result);
	unsigned int num_columns = mysql_num_fields(result);
	if (include_column_headers) ++num_rows;
	Table_ptr table = boost::make_shared<Table>(static_cast<size_t>(num_rows));

	// Get the column headers, if requested.
	if (include_column_headers) {
        while (MYSQL_FIELD *field = mysql_fetch_field(result)) {
			(*table)[0].push_back(UnicodeUtil::toUTF16StdString(field->name));
		}
	}

	// Get the data rows
	size_t row_num = include_column_headers ? 1 : 0;
	while (MYSQL_ROW row = mysql_fetch_row(result)) {
		TableRow &tableRow = (*table)[row_num];
		for (unsigned int i=0; i<num_columns; ++i) {
			if (row[i] == NULL)
				tableRow.push_back(L"");
			else
				tableRow.push_back(UnicodeUtil::toUTF16StdString(row[i]));
		}
		++row_num;
	}

	if (queryTimer) queryTimer->stop();
	return table;
}

struct MySQLDBConnection::MySQLRowIteratorCore: public DatabaseConnection::RowIteratorCore {
	MySQLRowIteratorCore(MySQLDBConnection::Impl *impl, const char* query)
		: result(0), current_row(0), num_columns(0), queryTimer(0), handle(impl->getHandle())
	{
		if (impl->queryProfiler) {
			queryTimer = _new QueryProfiler::QueryTimer(impl->queryProfiler->timerFor(query));
			queryTimer->start();
		}
		if (mysql_query(handle->conn, query))
			throwMySQLError(handle->conn, query);

		result = mysql_use_result(handle->conn);
		if (!result) return; // EOF iterator: no result (eg CREATE TABLE command)
		num_columns = mysql_num_fields(result);
		current_row = mysql_fetch_row(result);
		if (!current_row) {
			mysql_free_result(result);
			result = 0; // EOF iterator: empty table
			return;
		}
	}

	virtual ~MySQLRowIteratorCore() { 
		if (result) mysql_free_result(result);
		result = 0;
		current_row = 0;
		if (queryTimer) {
			queryTimer->stop();
			delete queryTimer;
		}
	}

	virtual const char* getCell(size_t column) {
		if (isEOF())
			throw UnexpectedInputException("MySQLDBConnection::MySQLRowIteratorCore::getCell",
				"Attempt to get cell from EOF iterator");
		if (column >= num_columns)
			throw UnexpectedInputException("MySQLDBConnection::MySQLRowIteratorCore::getCell",
				"Column number out of bounds");
		return current_row[column];
	}

	virtual size_t getNumColumns() {
		return num_columns;
	}

	virtual void fetchNextRow() {
		if (current_row == NULL)
			throw UnexpectedInputException("MySQLDBConnection::MySQLRowIteratorCore::fetchNextRow",
				"Already at end-of-table.");
		current_row = mysql_fetch_row(result);
		if (!current_row) {
			mysql_free_result(result);
			result = 0; // EOF iterator
		}
	}

	virtual bool isEOF() const { 
		return (result == 0) || (current_row == 0); 
	}

private:
	MySQLDBConnection::Impl::ActiveMySQLConnection_ptr handle;
	// Result:
	MYSQL_RES *result;
	MYSQL_ROW current_row;
	unsigned int num_columns;
	QueryProfiler::QueryTimer *queryTimer;
};

DatabaseConnection::RowIterator MySQLDBConnection::iter(const char* query) {
	return RowIterator(boost::shared_ptr<RowIteratorCore>(_new MySQLRowIteratorCore(_impl, query)));
}

