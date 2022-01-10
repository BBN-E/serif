// Copyright 2012 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/Symbol.h"
#include <vector>
#include <list>
#include <map>
#include <sstream>

/** An abstract base class used to provide a common interface to a
  * variety of database backends.  This class provides support for 
  * two basic access patterns: full table retrieval (via the "exec()" 
  * method) and row-by-row iteration (via the "iter()" method).
  *
  * DatabaseConnection objects are non-copyable.
  */
class DatabaseConnection: private boost::noncopyable {
protected:
	struct RowIteratorCore; // defined below.
	typedef boost::shared_ptr<RowIteratorCore> RowIteratorCore_ptr;
public:
	/** Return a connection to the database at the specified URL.  
	  * Currently supported URLs include:
	  * 
	  *   - sqlite://PATH
	  *   - sqlite://PATH?readonly
	  *   - sqlite://PATH?create
	  *   - mysql://HOST:PORT/DATABASE?user=USER&password=PASS
	  *   - postgresql://HOST:PORT/DATABASE?user=USER&password=PASS
	  *
	  * (mysql URLs require the mysql feature module, which is not
	  * included in standard SERIF builds because of licensing 
	  * issues. postgresql URLs require the postgres feature module.)
	  */
	static boost::shared_ptr<DatabaseConnection> connect(const char* url);
	static boost::shared_ptr<DatabaseConnection> connect(const std::string &s) { return connect(s.c_str()); }
	static boost::shared_ptr<DatabaseConnection> connect(const std::wstring &s) { return connect(UnicodeUtil::toUTF8StdString(s)); }

	/** Delete a database connection. */
	virtual ~DatabaseConnection();

	/** Return true if a table with the specified name exists **/
	virtual bool tableExists(const std::wstring& table_name) = 0;

	/** Begin a transaction.  Subsequent calls to exec() or iter()
	  * will be considered part of this transaction. */
	virtual void beginTransaction() = 0;

	/** Commit the current transaction. */
	virtual void endTransaction() = 0;

	/** Return true if this is a "read-only" connection. */
	virtual bool readOnly() const = 0;

	/** Turn on SQL query profiling.  This will track how much time
	  * is spent processing each SQL query, and will display results
	  * (in a log message) when the connection is destroyed. */
	virtual void enableProfiling() = 0;

	/** Return results from profiling. */
	virtual std::string getProfileResults() = 0;

	//====================== FULL TABLE RETRIEVAL ======================

	typedef std::vector<std::wstring> TableRow;
	typedef std::vector<TableRow> Table;
	typedef boost::shared_ptr<Table> Table_ptr;

	/** Run the specified SQL query, and return the result as a full table.
	  * Note that the entire result will be read into memory; if you expect
	  * that your query may return a large number of rows, then you may
	  * want to consider using DatabaseConnection::iter() instead. 
	  *
	  * Additionally, this method does not distinguish NULL cells from 
	  * empty cells -- both will be represented as empty strings. */
	virtual Table_ptr exec(const char* query, bool include_column_headers=false) = 0;
	virtual Table_ptr exec(const std::string &s, bool include_column_headers=false) { return exec(s.c_str(), include_column_headers); }
	virtual Table_ptr exec(const std::ostringstream &s, bool include_column_headers=false) { return exec(s.str(), include_column_headers); }
	virtual Table_ptr exec(const std::wstring &s, bool include_column_headers=false) { return exec(UnicodeUtil::toUTF8StdString(s), include_column_headers); }
	virtual Table_ptr exec(const std::wostringstream &s, bool include_column_headers=false) { return exec(s.str(), include_column_headers); }

	//====================== ROW ITERATOR ACCESS =======================

	class RowIterator;

	/** Run the specified SQL query, and return a "row iterator" that can
	  * be used to iterate over the results of the query.  Unlike the
	  * DatabaseConnection::exec() method, this method does *not* read the
	  * entire result into a memory.  This gives it a lower memory overhead
	  * (especially for large results) and makes it somewhat faster, though
	  * it may use more server resources. 
	  * 
	  * However, for most database backends, you may only run a single iter 
	  * query at a time. */
	virtual RowIterator iter(const char* query) = 0;
	virtual RowIterator iter(const std::string &s) { return iter(s.c_str()); }
	virtual RowIterator iter(const std::ostringstream &s) { return iter(s.str()); }
	virtual RowIterator iter(const std::wstring &s) { return iter(UnicodeUtil::toUTF8StdString(s)); }
	virtual RowIterator iter(const std::wostringstream &s) { return iter(s.str()); }

	virtual RowIterator end() { return RowIterator(); }

	/** Run the specified SQL query, and return a "row iterator" that can
	  * be used to iterate over the results of the query.  Unlike the basic
	  * DatabaseConnection::iter() method, this method uses an SQL "LIMIT"
	  * clause to retrieve a block of results at a time.  (The size of this
	  * block is determined by the "block_size" parameter).  This gives the reduced
	  * memory overhead of using iter(), but has the advantage that you can
	  * run mulitple queries at a time. */
	RowIterator iter_with_limit(const char* query, size_t block_size=1024);
	RowIterator iter_with_limit(const std::string &s, size_t block_size=1024) { return iter_with_limit(s.c_str(), block_size); }
	RowIterator iter_with_limit(const std::ostringstream &s, size_t block_size=1024) { return iter_with_limit(s.str(), block_size); }
	RowIterator iter_with_limit(const std::wstring &s, size_t block_size=1024) { return iter_with_limit(UnicodeUtil::toUTF8StdString(s), block_size); }
	RowIterator iter_with_limit(const std::wostringstream &s, size_t block_size=1024) { return iter_with_limit(s.str(), block_size); }

	/** Iterator class used to access results.  The iterator points at
	  * one row at a time.  Use getCell(i) and related methods to get
	  * column values from the current row. */
	class RowIterator {
	public:
		/** Move to the next row. */
		RowIterator &operator++();

		bool operator==(const RowIterator &other) const;
		bool operator!=(const RowIterator &other) const { return !operator==(other); }

		/** Return a vector containing the cells for the current row. */
		TableRow operator*();

		size_t getNumColumns();

		/** Return the specified cell, as a UTF-8 string (or NULL). */
		const char* getCell(size_t column);

		/** Return the specified cell, as a Symbol.  If the cell is 
		  * NULL, then return Symbol().*/
		Symbol getCellAsSymbol(size_t column);

		/** Return the specified cell, as a std::wstring.  If the cell
		  * is NULL, then return an empty string. */
		std::string getCellAsString(size_t column);

		/** Return the specified cell, as a std::wstring.  If the cell
		  * is NULL, then return an empty string. */
		std::wstring getCellAsWString(size_t column, bool die_on_utf_error=true);

		/** Return the specified cell as a double.  If
		  * the cell is NULL, then return the specified null_value. */
		double getCellAsDouble(size_t column, double null_value = -1.0);

		/** Return the specified cell as a 32-bit signed integer.  If
		  * the cell is NULL, then return the specified null_value. */
		boost::int32_t getCellAsInt32(size_t column, boost::int32_t null_value=-1);

		/** Return the specified cell as a 64-bit signed integer.  If
		  * the cell is NULL, then return the specified null_value. */
		boost::int64_t getCellAsInt64(size_t column, boost::int64_t null_value=-1);

		/** Return the specified cell as a boolean. If the cell is
		  * NULL, return false. */
		bool getCellAsBool(size_t column);
		
		/** Return true if the given cell is NULL. */
		bool isCellNull(size_t column);

		/** Return the "end-of-table" iterator. */
		RowIterator(): _impl() {}

		/** Return an iterator that wraps the given core. */
		RowIterator(RowIteratorCore_ptr impl): _impl(impl) {}
	protected:
		RowIteratorCore_ptr _impl;
		bool isEOF() const { return ((!_impl) || (_impl->isEOF())); }
	};

	//====================== QUOTING/SANITIZING =======================

	/** Return an SQL string with the given contents.  In particular, 
	  * return "'"+sql.replace("'", "''")+"'". */
	static std::wstring quote(const std::wstring &s);
	static std::string quote(const std::string &s);

	/** Replace every occurence of "'" in sql with "''", and return the 
	  * resulting string.  This is used when constructing SQL commands.
	  * In particular, when commands contain string values, those values
	  * should be passed through sanitize() and surrounded by single quotes. */
	static std::wstring sanitize(const std::wstring &s);
	static std::string sanitize(const std::string &s);

	/** Convert a vector of objects into a list suitable for inclusion in a
	 *  query, e.g. "(1, 2, 3, 4)"
	 */
	static std::string makeList(std::vector<int>& vec);
	static std::string makeList(std::list<int>& mylist);
	static std::string makeList(std::set<int>& myset);

	/** Convert the given string to a date, using whatever functions
	 * or formatting is appropriate for this databse.  Currently, the
	 * only supported formats for `s` are:
	 *   - YYYY-MM-DD
	 *   - DD-MON-YY
	 */
	virtual std::string toDate(const std::string &s) = 0;
	std::wstring toDate(const std::wstring &s) {
		return UnicodeUtil::toUTF16StdString(toDate(UnicodeUtil::toUTF8StdString(s)));
	}

	//====================== DATABASE TYPES =======================

	/** Return the name of the sql variant used by this database (eg
	 * "Oracle" or "MySQL").  This can be necessary when executing
	 * queries whose syntax is not standardized across different SQL
	 * variants (e.g. "CREATE TABLE"). */
	virtual std::string getSqlVariant() = 0;

	template<typename Implementation>
	static void registerImplementation(const char *url_scheme) {
		boost::shared_ptr<ImplementationRecord> implRecord(_new ImplRecordFor<Implementation>);
		_dbImplMap()[url_scheme] = implRecord;
	}
protected:
	/** Abstract base class for shared iterator "core" object that
	  * implements the basic functionality of the iterator.  Each
	  * DatabaseConnection subclass should define its own subclass
	  * of RowIteratorCore. */
	struct RowIteratorCore {
		virtual ~RowIteratorCore() {}
		virtual const char* getCell(size_t column) = 0;
		virtual void fetchNextRow() = 0;
		virtual bool isEOF() const = 0;
		virtual size_t getNumColumns() = 0;
		virtual std::wstring getCellAsWString(size_t column, bool die_on_utf_error);
	};

	virtual std::string rowLimitQuery(const std::string& query, size_t block_start, size_t block_size);

private:
	struct ImplementationRecord {
		virtual boost::shared_ptr<DatabaseConnection> connect(const std::string& host, int port,
			const std::string& database, const std::map<std::string,std::string> &parameters) = 0;
	};
	template <typename Implementation>
	struct ImplRecordFor: public ImplementationRecord {
		boost::shared_ptr<DatabaseConnection> connect(const std::string& host, int port,
			const std::string& database, const std::map<std::string,std::string> &parameters) {
			return boost::shared_ptr<DatabaseConnection>(_new Implementation(host, port, database, parameters));
		}
	};

	typedef std::map<std::string, boost::shared_ptr<ImplementationRecord> > DBImplMap;
	static DBImplMap &_dbImplMap();
	class LimitRowIterator;

};

typedef boost::shared_ptr<DatabaseConnection> DatabaseConnection_ptr;

#endif
