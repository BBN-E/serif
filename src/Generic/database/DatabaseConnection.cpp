// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/database/DatabaseConnection.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include "Generic/database/SqliteDBConnection.h"

DatabaseConnection_ptr DatabaseConnection::connect(const char* url) {
	std::string url_str(url);
	boost::replace_all(url_str, "\\", "/"); // the param reader mangles our url -- demangle it.
	static boost::regex url_re("(\\w+)://(([^:/]+)(:[0-9]+)?/)?([^\\?]+)(\\?.*)?");
	boost::smatch match;
	if (!boost::regex_match(url_str, match, url_re)) 
		throw UnexpectedInputException("DatabaseConnection::connect", "Bad database URL", url);

	std::string scheme = match.str(1);
	std::string host = match.str(3);
	std::string port_str = match.str(4);
	std::string dbname = match.str(5);
	std::string param_str = match.str(6);
	int port;
	try {
		if (!port_str.empty()) port_str = port_str.substr(1); // strip the leading ":"
		port = port_str.empty() ? 0 : boost::lexical_cast<int>(port_str);
	} catch (boost::bad_lexical_cast&) {
		throw UnexpectedInputException("DatabaseConnection::connect", 
			"Bad port number", port_str.c_str());
	}
	// Parse the extra parameters
	if (!param_str.empty()) param_str = param_str.substr(1); // strip the leading "?"
	std::map<std::string, std::string> parameters;
	std::vector<std::string> param_strs;
	boost::split(param_strs, param_str, boost::is_any_of("&"));
	BOOST_FOREACH(std::string pstr, param_strs) {
		std::string::size_type split = pstr.find("=");
		if (split == std::string::npos)
			parameters.insert(std::make_pair(pstr.substr(0, split), std::string("true")));
		else
			parameters.insert(std::make_pair(pstr.substr(0, split), pstr.substr(split+1)));
	}
	DBImplMap& dbImplMap = _dbImplMap();
	DBImplMap::iterator it = dbImplMap.find(scheme);
	if (it == dbImplMap.end()) {
		std::ostringstream err;
		err << "Unknown scheme \"" << scheme << "\" in URL \"" << url 
			<< "\".  Note that you may need to use feature modules "
			<< "to enable some schemes (such as mysql, which requires "
			<< "the MySQL feature module).";
		throw UnexpectedInputException("DatabaseConnection::connect", err.str().c_str());
	}
	return (*it).second->connect(host, port, dbname, parameters);
}

DatabaseConnection::~DatabaseConnection() {}


DatabaseConnection::DBImplMap &DatabaseConnection::_dbImplMap() {
	static DBImplMap dbImplMap;
	if (dbImplMap.empty()) {
		boost::shared_ptr<ImplementationRecord> implRecord(_new ImplRecordFor<SqliteDBConnection>);
		dbImplMap["sqlite"] = implRecord;
	}
	return dbImplMap;
}

bool DatabaseConnection::RowIterator::operator==(const RowIterator &other) const {
	// Two iterators are the same if they have the same underlying core, or
	// if they are both EOF iterators.
	return ((_impl == other._impl) || (isEOF() && other.isEOF()));
}

bool DatabaseConnection::RowIterator::isCellNull(size_t column) {
	return (_impl->getCell(column) == NULL);
}

const char* DatabaseConnection::RowIterator::getCell(size_t column) {
	return _impl->getCell(column);
}

size_t DatabaseConnection::RowIterator::getNumColumns() {
	return _impl->getNumColumns();
}

Symbol DatabaseConnection::RowIterator::getCellAsSymbol(size_t column) {
	const char* cell = getCell(column);
	return cell?Symbol(UnicodeUtil::toUTF16StdString(cell)):Symbol();
}

std::wstring DatabaseConnection::RowIterator::getCellAsWString(size_t column, bool die_on_utf_error) {
	return _impl->getCellAsWString(column, die_on_utf_error);
}

std::string DatabaseConnection::RowIterator::getCellAsString(size_t column) {
	const char* cell = getCell(column);
	return cell?cell:"";
}

double DatabaseConnection::RowIterator::getCellAsDouble(size_t column, double null_value) {
	const char* cell = getCell(column);
	try {
		if (!cell || strlen(cell) == 0)
			return null_value;
		return boost::lexical_cast<double>(cell);
	} catch (boost::bad_lexical_cast &) {
		throw UnexpectedInputException("DatabaseConnection::RowIterator::getCellAsDouble", 
			"Expected a double, got ", cell);
	}	
}

boost::int32_t DatabaseConnection::RowIterator::getCellAsInt32(size_t column, boost::int32_t null_value) {
	const char* cell = getCell(column);
	try {
		if (!cell || strlen(cell) == 0)
			return null_value;
		return boost::lexical_cast<boost::int32_t>(cell);
	} catch (boost::bad_lexical_cast &) {
		throw UnexpectedInputException("DatabaseConnection::RowIterator::getCellAsInt32", 
			"Expected an int32, got ", cell);
	}	
}

boost::int64_t DatabaseConnection::RowIterator::getCellAsInt64(size_t column, boost::int64_t null_value) {
	const char* cell = getCell(column);
	try {
		if (!cell || strlen(cell) == 0)
			return null_value;
		return boost::lexical_cast<boost::int64_t>(cell);
	} catch (boost::bad_lexical_cast &) {
		throw UnexpectedInputException("DatabaseConnection::RowIterator::getCellAsInt64", 
			"Expected an int64, got ", cell);
	}
}

bool DatabaseConnection::RowIterator::getCellAsBool(size_t column) {
	const char* cell = getCell(column);
	if (!cell)
		return false;
	
	if (strcmp(cell, "f") == 0)
		return false;

	// sqlite stores booleans as 0/1
	if (strcmp(cell, "0") == 0)
		return false;

	return true;
}

DatabaseConnection::RowIterator &DatabaseConnection::RowIterator::operator++() {
	_impl->fetchNextRow();
	if (_impl->isEOF()) {
		_impl.reset();
	}
	return *this;
}

std::wstring DatabaseConnection::sanitize(const std::wstring &s)
{
	return boost::algorithm::replace_all_copy(s, L"'", L"''");
}

std::string DatabaseConnection::sanitize(const std::string &s)
{
	return boost::algorithm::replace_all_copy(s, "'", "''");
}

std::string DatabaseConnection::makeList(std::vector<int>& vec) {
	std::stringstream result;
	result << "(";
	for (size_t i = 0; i < vec.size(); i++) {
		if (i != 0)
			result << ", ";
		result << vec.at(i);
	}
	result << ")";
	return result.str();
}
std::string DatabaseConnection::makeList(std::set<int>& myset) {
	std::stringstream result;
	result << "(";
	bool begin = true;
	BOOST_FOREACH(int i, myset) {
		if (!begin)
			result << ", ";
		result << i;
		begin = false;
	}
	result << ")";
	return result.str();
}
std::string DatabaseConnection::makeList(std::list<int>& mylist) {
	std::stringstream result;
	result << "(";
	bool begin = true;
	BOOST_FOREACH(int i, mylist) {
		if (!begin)
			result << ", ";
		result << i;
		begin = false;
	}
	result << ")";
	return result.str();
}

std::wstring DatabaseConnection::quote(const std::wstring &s)
{
	return L'\'' + boost::algorithm::replace_all_copy(s, L"'", L"''") + L'\'';
}

std::string DatabaseConnection::quote(const std::string &s)
{
	return '\'' + boost::algorithm::replace_all_copy(s, "'", "''") + '\'';
}

std::wstring DatabaseConnection::RowIteratorCore::getCellAsWString(size_t column, bool die_on_utf_error)
{
	const char* cell = getCell(column);
	UnicodeUtil::ErrorResponse errorResponse = die_on_utf_error?UnicodeUtil::DIE_ON_ERROR:UnicodeUtil::REPLACE_ON_ERROR;
	return UnicodeUtil::toUTF16StdString(cell?cell:"", errorResponse);
}

class DatabaseConnection::LimitRowIterator: public DatabaseConnection::RowIteratorCore {
public:
	LimitRowIterator(DatabaseConnection *db, std::string query, size_t block_size)
	: db(db), query(query), block_size(block_size), block_start(0), pos(0) 
	{
		rows.reserve(block_size);
		loadBlock();
	}
	virtual void fetchNextRow() {
		if (pos < (block_size-1)) {
			++pos;
		} else {
			loadBlock();
		}
	}

	virtual const char* getCell(size_t column) {
		return rows[pos][column].second;
	}

	virtual bool isEOF() const {
		return (pos >= rows.size());
	}

	virtual size_t getNumColumns() {
		return rows[pos].size();
	}

private:
	std::string query;
	DatabaseConnection *db;
	size_t block_size;
	size_t block_start;
	size_t pos;
	typedef std::pair<std::string, const char*> Cell;
	typedef std::vector<Cell> Row;
	std::vector<Row> rows;
	void loadBlock() {
		rows.clear();
		std::string limited_query = db->rowLimitQuery(query, block_start, block_size);
		for (DatabaseConnection::RowIterator rowIter = db->iter(limited_query); rowIter!=db->end(); ++rowIter) {
			size_t n_columns = rowIter.getNumColumns();
			rows.push_back(Row(n_columns));
			Row& row = rows.back();
			for (size_t c=0; c<n_columns; ++c) {
				if (rowIter.isCellNull(c)) {
					row[c].second = 0;
				} else {
					row[c].first = rowIter.getCell(c);
					row[c].second = row[c].first.c_str();
				}
			}
		}
		if (rows.size() > block_size)
			throw InternalInconsistencyException("LimitRowIterator::loadBlock", 
			    "SQL query with LIMIT returned more results than expected!");
		pos = 0;
		block_start += block_size;
	}
};

DatabaseConnection::RowIterator DatabaseConnection::iter_with_limit(const char* query, size_t block_size) {
	return RowIterator(boost::make_shared<LimitRowIterator>(this, query, block_size));
}

std::string DatabaseConnection::rowLimitQuery(const std::string& query, size_t block_start, size_t block_size) {
	std::ostringstream limited_query;
	limited_query << query << " LIMIT " << block_start << ", " << block_size;
	return limited_query.str();
}




//////////////////// MYSQL ///////////////////////////







