// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/database/SqliteDBConnection.h"
#include "Generic/database/QueryProfiler.h"
#include "Generic/sqlite/sqlite3.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

SqliteDBConnection::SqliteDBConnection(const std::string& host, int port,
		const std::string& database, const std::map<std::string,std::string> &parameters)
: _readOnly(false), _queryProfiler(0), _localTempCopy()
{
	if (!host.empty()) {
		std::wstringstream wss;
		wss << "sqlite URLs should not contain a host: " << std::wstring(host.begin(), host.end());
		throw UnexpectedInputException("SqliteDBImplementation::connect", wss);
	}
	if (port != 0) {
		std::wstringstream wss;
		wss << "sqlite URLs should not contain a port: " << port;
		throw UnexpectedInputException("SqliteDBImplementation::connect", wss);
	}
	std::map<std::string,std::string> piter;
	bool create = (parameters.find("create") != parameters.end());
	bool readonly = (parameters.find("readonly") != parameters.end());
	bool copy = (parameters.find("copy") != parameters.end());

	if (copy && boost::filesystem::exists(database.c_str())) {
		if (!readonly)
			throw UnexpectedInputException("SqliteDBImplementation::connect",
				"The 'copy' option may only be used if 'readonly' is also used");
		// Get a temporary file.
		OutputUtil::NamedTempFile tempFile = OutputUtil::makeNamedTempFile();
		try {
			// Copy the database.
			SessionLogger::info("Sqlite") << "Creating temporary copy of " << database 
				<< " in " << tempFile.first;
			std::ifstream src(database.c_str(), std::fstream::binary);
			*(tempFile.second) << src.rdbuf();
			tempFile.second->close();
			// Connect using the temporary copy:
			connect(tempFile.first, readonly, create);
			// Save the filename so we can delete it later:
			_localTempCopy = tempFile.first;
		} catch (...) {
			remove(tempFile.first.c_str());
			throw;
		}
	} else {
		connect(database, readonly, create);
	}
	if (parameters.find("cache_size") != parameters.end()) {
		// E.g.: PRAGMA cache_size=5000  (default=2000)
		std::ostringstream query;
		query << "PRAGMA cache_size=" << parameters.find("cache_size")->second;
		exec(query.str().c_str());
	}
	if (parameters.find("temp_store") != parameters.end()) {
		// E.g.: PRAGMA temp_store = MEMORY
		std::ostringstream query;
		query << "PRAGMA temp_store=" << parameters.find("temp_store")->second;
		exec(query.str().c_str());
	}
}


SqliteDBConnection::SqliteDBConnection (const std::string& db_location, bool readonly,bool create) 
: _readOnly(readonly) , _queryProfiler(0), _localTempCopy()
{
	connect(db_location, readonly, create);
}

void SqliteDBConnection::connect(const std::string& db_location, bool readonly, bool create) 
{
	_readOnly = readonly;
	int flags = readonly?SQLITE_OPEN_READONLY:SQLITE_OPEN_READWRITE;
	if (create) {
		if (!readonly) {
			flags = flags | SQLITE_OPEN_CREATE;
		} else {
			SessionLogger::warn("incompatible_db_flags") << 
				"SqliteDBConnection::SqliteDBConnection: Read-only and create flags to SQLite database "
				"opening are incompatible. Ignoring create flag.";
		}
	}

	int rc = sqlite3_open_v2(db_location.c_str(), &_db, flags, NULL);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
		sqlite3_close(_db);
		std::stringstream err;
		err << "Can't open database: " << db_location;
		throw UnexpectedInputException("SqliteDBConnection::connect", err.str().c_str());
    }
}

SqliteDBConnection::~SqliteDBConnection() {
	if (_queryProfiler) {
		SessionLogger::info("SQL") << _queryProfiler->getResults();
		delete _queryProfiler;
	}
    sqlite3_close(_db);
	if (!_localTempCopy.empty()) {
		SessionLogger::info("Sqlite") << "Deleting temporary database cache " << _localTempCopy;
		remove(_localTempCopy.c_str());
		_localTempCopy.clear();
	}
} 

void SqliteDBConnection::enableProfiling() {
	if (!_queryProfiler)
		_queryProfiler = _new QueryProfiler();
}

std::string SqliteDBConnection::getProfileResults() {
	if (_queryProfiler) {
		return _queryProfiler->getResults();
	} 
	return "";
}

DatabaseConnection::Table_ptr SqliteDBConnection::exec(const char* utf8_sql_command, bool include_column_headers) {
	// Output variables for sqlite3_get_table:
	char **result=0;      // Table of string values
	int nrow=0;           // Number of rows
	int	ncol=0;           // Number of columns
	char *zErrMsg=0;      // Error message

	// Start profiler (if enabled)
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_queryProfiler->timerFor(utf8_sql_command));
	if (queryTimer) queryTimer->start();

	// Run the command.
	int rc = sqlite3_get_table(_db, utf8_sql_command, &result, &nrow, &ncol, &zErrMsg);

	// Report any errors:
	if (rc != SQLITE_OK) {
		std::cerr << utf8_sql_command << std::endl;
		throw UnexpectedInputException("SqliteDBConnection::exec", zErrMsg);
	}

	// What row should we start copying the table on?  If we're not including the
	// column headers, then skip the first row (i.e., start at i=1).
	int start = (include_column_headers?0:1);

	// Copy the result our C++ table.
	Table_ptr table = boost::make_shared<Table>(nrow-start+1);
	for (int i=start; i<nrow+1; ++i) {
		for (int j=0; j<ncol; ++j) {
			if (result[i*ncol+j] == NULL)
				(*table)[i-start].push_back(L"");
			else
				(*table)[i-start].push_back(UnicodeUtil::toUTF16StdString(result[i*ncol+j]));
		}
	}

	// Free sqlite's copy of the table.
    sqlite3_free_table(result);

	// Stop profiler (if enabled)
	if (queryTimer) queryTimer->stop();

	return table;
}

struct SqliteDBConnection::SqliteRowIteratorCore: public DatabaseConnection::RowIteratorCore {
private:
	char **result;      // Table of string values
	int nrow;           // Number of rows
	int	ncol;           // Number of columns
	int current_row;
	QueryProfiler::QueryTimer *queryTimer;
public:
	SqliteRowIteratorCore(sqlite3 *db, const char* utf8_sql_command, QueryProfiler::QueryTimer *queryTimer)
		: nrow(0), ncol(0), result(0), current_row(0), queryTimer(queryTimer)
	{
		// Start profiler (if enabled)
		if (queryTimer) queryTimer->start();

		char *zErrMsg=0;      // Error message
		int rc = sqlite3_get_table(db, utf8_sql_command, &result, &nrow, &ncol, &zErrMsg);
		if (rc != SQLITE_OK) {
			std::cerr << utf8_sql_command << std::endl;
			throw UnexpectedInputException("SqliteDBConnection::exec", zErrMsg);
		}
	}

	virtual ~SqliteRowIteratorCore() { 
		// Stop profiler (if enabled)
		if (queryTimer) {
			queryTimer->stop();
			delete queryTimer;
		}

		if (result) sqlite3_free_table(result);
		result = 0;
	}

	virtual const char* getCell(size_t column) {
		if (isEOF())
			throw UnexpectedInputException("SqliteDBConnection::SqliteRowIteratorCore::getCell",
				"Attempt to get cell from EOF iterator");
		if (static_cast<int>(column) >= ncol)
			throw UnexpectedInputException("SqliteDBConnection::SqliteRowIteratorCore::getCell",
				"Column number out of bounds");
		// Add 1 to the row to skip the header.
		return result[(1+current_row)*ncol+column];
	}

	virtual size_t getNumColumns() {
		return ncol;
	}

	virtual void fetchNextRow() {
		if (isEOF())
			throw UnexpectedInputException("SqliteDBConnection::SqliteRowIteratorCore::fetchNextRow",
				"Already at end-of-table.");
		++current_row;
	}

	virtual bool isEOF() const { 
		return (result == 0) || (current_row >= nrow); 
	}
};

DatabaseConnection::RowIterator SqliteDBConnection::iter(const char* query) {
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_queryProfiler->timerFor(query));
	return RowIterator(boost::shared_ptr<RowIteratorCore>(_new SqliteRowIteratorCore(_db, query, queryTimer)));
}


void SqliteDBConnection::beginTransaction() {
	sqlite3_exec(_db, "BEGIN", 0, 0, 0);
}

void SqliteDBConnection::endTransaction() {
	sqlite3_exec(_db, "COMMIT", 0, 0, 0);
}

bool SqliteDBConnection::tableExists(const std::wstring & table_name) {
	std::ostringstream query;
	query << "select 1 from sqlite_master where type='table' and name = " 
		<< quote(table_name);
	return iter(query) != end();
}

bool SqliteDBConnection::readOnly() const {
	return _readOnly;
}

namespace {
	std::string convertMonth(const std::string &abbrev) {
		static std::map<std::string, std::string> monthTable;
		if (monthTable.empty()) {
			monthTable["JAN"] = "01";
			monthTable["FEB"] = "02";
			monthTable["MAR"] = "03";
			monthTable["APR"] = "04";
			monthTable["MAY"] = "05";
			monthTable["JUN"] = "06";
			monthTable["JUL"] = "07";
			monthTable["AUG"] = "08";
			monthTable["SEP"] = "09";
			monthTable["OCT"] = "10";
			monthTable["NOV"] = "11";
			monthTable["DEC"] = "12";
		}
		std::map<std::string, std::string>::iterator month = monthTable.find(boost::to_upper_copy(abbrev));
		if (month != monthTable.end())
			return month->second;
		else
			return "";
	}
}

std::string SqliteDBConnection::toDate(const std::string &s) {
	static boost::regex YYYY_MM_DD("^([0-9][0-9][0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9])");
	static boost::regex MM_DD_YYYY("^([0-9][0-9])[\\-/]([0-9][0-9])[\\-/]([0-9][0-9][0-9][0-9])");
	static boost::regex DD_MON_YY("^([0-9][0-9])[\\-/]([A-Za-z][A-Za-z][A-Za-z])[\\-/]([0-9][0-9])");
	boost::smatch match;

	// We need to return dates enclosed in 'single quotes' if they are to be used in queries.
	std::ostringstream out;

	// This is sqlite's native format.
	if (boost::regex_match(s, YYYY_MM_DD)) {
		out << "'" << s << "'";
		return out.str();
	}

	if (boost::regex_match(s, match, MM_DD_YYYY)) {
		out << "'" << match[2] << "-" << match[3] << "-" << match[1] << "'";
		return out.str();
	}

	if (boost::regex_match(s, match, DD_MON_YY)) {
		int year = boost::lexical_cast<int>(match[3]);
		std::string month = convertMonth(match[2]);
		if (!month.empty()) {
			out << "'"
				<< ((year<50)?(2000+year):(1900+year)) << "-"
				<< month << "-" 
				<< match[1]
				<< "'";
			return out.str();
		}
	}

	// Unable to parse.
	SessionLogger::err("ICEWS") << "SqliteDBConnection::toDate() was passed a date in an unexpected format (" << s << "); NULL is being returned";
	return "null";
}

