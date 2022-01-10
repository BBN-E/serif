// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Postgres/PostgresDBConnection.h"
#include "Generic/database/QueryProfiler.h"
#include "Generic/common/UnsupportedOperationException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
//for libpqxx
//#include <pqxx/pqxx>
using namespace pqxx;



PostgresDBConnection::PostgresDBConnection(const std::string& host, int port, const std::string& database, 
		const std::string& username, const std::string& password)
{
	std::string conn_str = std::string("user=")+username+std::string(" host=")+host+
		std::string(" password=")+password+std::string(" dbname=")+database
		+std::string(" port=")+boost::lexical_cast<std::string>(port);
	try{
		conn = new connection(conn_str);
	}catch(std::exception &e){
		throw UnexpectedInputException("PostgresDBConnection::Constructor", e.what());
	}
	//setting _queryProfiler to NULL;
	_queryProfiler = 0;
}

PostgresDBConnection::PostgresDBConnection(const std::string& host, int port, const std::string& database, 
		const std::map<std::string,std::string> &parameters)
{
	if (parameters.count("user")==0 || parameters.count("password")==0)
		throw UnexpectedInputException("PostgresDBConnection::Constructor", "keys 'user' and 'password' not found in the input arg map 'parameters'");
	std::string username = parameters.find("user")->second;
	std::string password = parameters.find("password")->second;
	std::string conn_str = std::string("user=")+username+std::string(" host=")+host+
		std::string(" password=")+password+std::string(" dbname=")+database
		+std::string(" port=")+boost::lexical_cast<std::string>(port);
	try{
		conn = new connection(conn_str);
	}catch(std::exception &e){
		throw UnexpectedInputException("PostgresDBConnection::Constructor", e.what());
	}
	//setting _queryProfiler to NULL;
	_queryProfiler = 0;
}

PostgresDBConnection::~PostgresDBConnection() {
	if (_queryProfiler) {
		SessionLogger::info("SQL") << _queryProfiler->getResults();
		delete _queryProfiler;
	}
	conn->disconnect();
} 

void PostgresDBConnection::enableProfiling() {
	if (!_queryProfiler)
		_queryProfiler = _new QueryProfiler();
}

std::string PostgresDBConnection::getProfileResults() {
	if (_queryProfiler) {
		return _queryProfiler->getResults();
	} 
	return "";
}

DatabaseConnection::Table_ptr PostgresDBConnection::exec(const char* utf8_sql_command, bool include_column_headers) {
	if (include_column_headers) {
		std::cerr<<"Including column headers is not implemented for PostgresSQL.";
		throw UnsupportedOperationException("PostgresDBConnection::exec","Including column headers is not implemented for PostgresSQL.");
	}

	// Start profiler (if enabled)
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_queryProfiler->timerFor(utf8_sql_command));
	if (queryTimer) queryTimer->start();
	// Run the command.
	result result;
	try{
		//creating a nontransaction, since write operations are not expected
		nontransaction txn(*conn);
		result = txn.exec(utf8_sql_command);
	}catch(std::exception &e){
		std::stringstream errstr;
		errstr << "Failed query: " << utf8_sql_command << ";\nERROR = " << e.what();
		throw UnexpectedInputException("PostgresDBConnection::exec", errstr.str().c_str());
	}

	int nrow=result.size();           // Number of rows
	int	ncol=result.columns();           // Number of columns
	// Copy the result to our C++ table.
	Table_ptr table = boost::make_shared<Table>(nrow);
	for (int i=0; i<nrow; i++) {
		for (int j=0; j<ncol; j++) {
			const char* columnVal = result[i][j].as<const char*>();
			if (columnVal == NULL){
				(*table)[i].push_back(L"");
			}
			else{
				/*wchar_t* wideCol = new wchar_t[];
				UnicodeUtil::copyCharToWChar(columnVal,wideCol,strlen(columnVal)+1);
				const wchar_t* constWideCol = (const wchar_t*)wideCol;
				char* utf8ColumnVal = UnicodeUtil::toUTF8String(constWideCol);
				wchar_t* wideUtf8Col = new wchar_t[];
				UnicodeUtil::copyCharToWChar(utf8ColumnVal,wideUtf8Col,strlen(utf8ColumnVal)+1);
				std::wstring wideUtf8String = std::wstring(wideUtf8Col);
				std::wcout<<"wideutf8str="<<wideUtf8String;
				(*table)[i].push_back(wideUtf8String);*/
				(*table)[i].push_back(UnicodeUtil::toUTF16StdString(std::string(columnVal),UnicodeUtil::DIE_ON_ERROR));
			}
		}
	}
	
	//memory will be freed automatically by libpqxx

	// Stop profiler (if enabled)
	if (queryTimer) queryTimer->stop();
	return table;
}

struct PostgresDBConnection::PostgresRowIteratorCore: public DatabaseConnection::RowIteratorCore {
private:
	wchar_t ***results; //copy of the data retrieved from the db 
	int nrow;           // Number of rows
	int	ncol;           // Number of columns
	int current_row;
	QueryProfiler::QueryTimer *queryTimer;
	PostgresDBConnection *pgConn;
public:
	PostgresRowIteratorCore(const char* utf8_sql_command, PostgresDBConnection *pgConn, QueryProfiler::QueryTimer *queryTimer)
		: nrow(0), ncol(0), results(0), current_row(0), queryTimer(queryTimer), pgConn(pgConn)
	{
		result db_result;
		// Start profiler (if enabled)
		if (queryTimer) queryTimer->start();
	
		// Run the command.

		try{
			//creating a nontransaction, since write operations are not expected
			nontransaction txn(*(pgConn->conn));
			db_result = txn.exec(utf8_sql_command);
		}catch(std::exception &e){
			std::stringstream errstr;
			errstr << "Failed query: " << utf8_sql_command << ";\nERROR = " << e.what();
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::constructor", errstr.str().c_str());
		}

		nrow=db_result.size();           // Number of rows
		ncol=db_result.columns();           // Number of columns

		if (nrow != 0 && ncol != 0) {

			// Copy the result to our result array
			results = new wchar_t**[nrow];
			for (int i=0; i<nrow; i++) {
				results[i]=new wchar_t*[ncol];
				for (int j=0; j<ncol; j++) {
					if (db_result[i][j].as<const char*>() == NULL)
						results[i][j]=UnicodeUtil::toUTF16String("");
					else
						results[i][j]=UnicodeUtil::toUTF16String(db_result[i][j].as<const char*>(),UnicodeUtil::DIE_ON_ERROR);
				}
			}

		}
	}

	virtual ~PostgresRowIteratorCore() { 
		// Stop profiler (if enabled)
		if (queryTimer) {
			queryTimer->stop();
			delete queryTimer;
		}
		results = 0;
	}

	virtual const char* getCell(size_t column) {
		if (isEOF())
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::getCell",
				"Attempt to get cell from EOF iterator");
		if (static_cast<int>(column) >= ncol)
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::getCell",
				"Column number out of bounds");
		char* cellVal = new char[wcslen(results[current_row][column]) + 2];
		UnicodeUtil::copyWCharToChar(results[current_row][column],cellVal,wcslen(results[current_row][column]) + 1);
		return cellVal;
	}

	virtual size_t getNumColumns() {
		return ncol;
	}

	virtual void fetchNextRow() {
		if (isEOF())
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::fetchNextRow",
				"Already at end-of-table.");
		++current_row;
	}

	virtual bool isEOF() const { 
		return (results == 0) || (current_row >= nrow); 
	}

	virtual std::wstring getCellAsWString(size_t column, bool)
	{
		if (isEOF())
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::getCellAsWString",
				"Attempt to get cell from EOF iterator");
		if (static_cast<int>(column) >= ncol)
			throw UnexpectedInputException("PostgresDBConnection::PostgresRowIteratorCore::getCellAsWString",
				"Column number out of bounds");
		
		wchar_t *result = results[current_row][column];

		if (!result)
			return L"";
		return result;
	}
};

DatabaseConnection::RowIterator PostgresDBConnection::iter(const char* query) {
	QueryProfiler::QueryTimer *queryTimer = 0;
	if (_queryProfiler) queryTimer = _new QueryProfiler::QueryTimer(_queryProfiler->timerFor(query));
	return RowIterator(boost::shared_ptr<RowIteratorCore>(_new PostgresRowIteratorCore(query, this, queryTimer)));
}


void PostgresDBConnection::beginTransaction() {
	throw UnsupportedOperationException("PostgresDBConnection::endTransaction","No need to make calls to "
		" beginTransaction or endTransaction for Postgres DB");
}

void PostgresDBConnection::endTransaction() {
	throw UnsupportedOperationException("PostgresDBConnection::endTransaction","No need to make calls to "
		" beginTransaction or endTransaction for Postgres DB");
}

bool PostgresDBConnection::tableExists(const std::wstring & table_name) {
	std::cerr<<"The call made to PostgresDBConnection::tableExists with arg "<<table_name<<" failed..."<<std::endl;
	throw UnsupportedOperationException("PostgresDBConnection::tableExists","Method not supported"
		"for Postgres DB");
}

bool PostgresDBConnection::readOnly() const {
	/*This may not be very relevant for Postgres DB. But, returning true (unconditionally) now
	since SERIF is not supposed to write to the db (at least for now)
	*/
	return true;
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

std::string PostgresDBConnection::toDate(const std::string &s) {
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
	SessionLogger::err("ICEWS") << "PostgresDBConnection::toDate() was passed a date in an unexpected format (" << s << "); NULL is being returned";
	return "null";
}


std::string PostgresDBConnection::escape_and_quote(const std::string &s)
{
	//connection.quote does both the escaping and quoting
	return (*conn).quote(s);
}

