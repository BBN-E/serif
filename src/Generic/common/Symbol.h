// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

/** "Symbols" are interned strings.  In other words, Symbols are thin wrappers for
  * strings where we guarantee that exactly one Symbol is defined for each string
  * value.  This allows the symbols to be compared using simple (and fast) pointer
  * identity comparisons.  It also saves memory by preventing us from keeping multiple
  * copies of the same string.  For more information on string interning, see:
  * <http://en.wikipedia.org/wiki/String_interning>
  *
  * In order to guarantee Symbol uniqueness, we maintain a table containing all 
  * Symbols that have been constructed so far.  When the Symbol constructor is 
  * called, it checks this table for the requested string.  If it is found, then
  * it returns (a thin wrapper for) the existing copy.  Otherwise, it creates a new
  * interned string, adds it to the table, and returns (a thin wrapper for) it.
  * This table of symbols is implemented using the class SymbolTable.  By default,
  * Symbols are constructed with the global static table that is returned by 
  * Symbol::defaultSymbolTable().  
  * 
  * It is possible to have multiple SymbolTables.  A Symbol created with one SymbolTable
  * will never compare equal with a Symbol from a different SymbolTable (even if
  * their string contents match).  When a SymbolTable is destroyed, all Symbols
  * that it contains are destroyed as well.  Thus, a SymbolTable should never be
  * destroyed while there are still any pointers or references to Symbols contained
  * in that table.  
  *
  * Symbol objects are automatically reference-counted.  However, they will not be
  * automatically deleted when their reference count drops to zero.  This helps 
  * avoid unnecessary reallocations in cases where the same symbol is seen again.
  * If you wish to free the memory associated with Symbols whose reference counts
  * are zero, then you must explicitly call Symbol::discardUnusedSymbols() (for the
  * default global static symbol table) or SymbolTable::discardUnusedSymbols() (for
  * non-static symbol tables).  Reference counting can be turned off by unsetting
  * the preprocessor symbol SYMBOL_REF_COUNT (controlled by cmake).
  *
  * Basic Symbol operations (including symbol creation, deletion, and comparison)
  * are thread-safe iff the preprocessor symbol SYMBOL_THREADSAFE is defined.  Some
  * of the more complex operations, such as Symbol::discardUnusedSymbols(), are 
  * not thread-safe, and should only be used when you are sure a single thread is
  * running.  (SYMBOL_THREADSAFE is controlled by cmake.)
  */

#ifndef NEW_SYMBOL_H
#define NEW_SYMBOL_H

#include "Generic/common/hash_set.h"
#include "Generic/common/hash_map.h"
#include <boost/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/functional/hash.hpp>
#include "dynamic_includes/common/SymbolDefinitions.h"
#include <set>

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

// Forward declarations.
class Symbol;
class SymbolTable;
class SymbolTableImpl;
struct EqualSymbolDataPtr;
struct HashSymbolDataPtr;

/*#################################################################################*/
/** The internal representation of a Symbol.
  *
  * Two Symbols are considered equal iff they are wrappers for the same SymbolData 
  * object.  SymbolData keeps track of the string contents of the Symbol; its hash
  * value; a reference count; and a "debug string", which is an ASCII version of 
  * str.  The debug string pointer is initialized to NULL, and is only computed if 
  * it is requested (in which case it is cached for future use). 
  *
  * SymbolData is a Plain-Old-Data (POD) struct.  As a result, we don't have to 
  * worry about calling its destructor when we deallocate it.  For more info,
  * see: <http://en.wikipedia.org/wiki/Plain_Old_Data_Structures> 
  */
struct SymbolData: boost::noncopyable {
	const wchar_t* str;        // String contents of the interned Symbol.
	const char* debug_str;     // ASCII version of str; only allocated if requested.
	size_t hash_value;         // Cached value of Symbol::hash_str(this->str).
#ifdef SYMBOL_REF_COUNT
	boost::uint32_t ref_count; // Reference count.
#endif
};

#if !defined(SYMBOL_REF_COUNT)
#define SYMBOL_DATA_INCREF(s)
#define SYMBOL_DATA_DECREF(s)
#elif defined(SYMBOL_THREADSAFE)
#if BOOST_VERSION < 104800
#define SYMBOL_DATA_INCREF(s) (s ? boost::interprocess::detail::atomic_inc32(&((s)->ref_count)) : 0)
#define SYMBOL_DATA_DECREF(s) (s ? boost::interprocess::detail::atomic_dec32(&((s)->ref_count)) : 0)
#else 
#define SYMBOL_DATA_INCREF(s) (s ? boost::interprocess::ipcdetail::atomic_inc32(&((s)->ref_count)) : 0)
#define SYMBOL_DATA_DECREF(s) (s ? boost::interprocess::ipcdetail::atomic_dec32(&((s)->ref_count)) : 0)
#endif // BOOST_VERSION < 104800
#else
#define SYMBOL_DATA_INCREF(s) (s ? ++((s)->ref_count) : 0)
#define SYMBOL_DATA_DECREF(s) (s ? --((s)->ref_count) : 0)
#endif

/*#################################################################################*/
/** A table of Symbols (aka a "string interning pool").
  * 
  * The SymbolTable class has two functions: it provides a quick lookup table
  * for SymbolData objects (allowing us to guarantee that a single SymbolData 
  * is used for each string); and it manages the memory associated with 
  * SymbolsData (including their strings and debug strings).  
  * 
  * Note that the SymbolTable contains SymbolData objects, not Symbos themselves: 
  * Symbol objects are simply thin wrappers for SymbolData objects.
  */
class SymbolTable: boost::noncopyable {
	friend class Symbol;
public:
	SymbolTable(std::string const &tableName);
	virtual ~SymbolTable();

	/** Free the memory associated with any SymbolData objects whose reference
	  * count is zero. */
	size_t discardUnusedSymbols(bool verbose=false);

	/** Free the memory associated with all symbols.  It is an error to attempt 
	  * to access a symbol in this SymbolTable after this is called (including
	  * the NULL symbol) */
	size_t discardAllSymbols();

	/** Return true if this SymbolTable contains a SymbolData object for the
	  * given string. */
	bool contains(const wchar_t *str);

	/** Initialize the Symbol table with common words, read from the file
	  * ParamReader::getParam("symbol_table_initialization_file").  This will
	  * permanantly intern all strings contained in that file (i.e., their 
	  * reference count will never go to zero, so they will not be deleted by 
	  * discardUnusedSymbols). */
	void initializeSymbolsFromFile();

	/** Return the number of Symbols interned in this SymbolTable. */
	size_t size();

	/** Display debugging information about this SymbolTable, including 
	  * approximate memory usage statistics. */
	void printDebugInfo();

	/** Release the memory associated with all debug strings for symbols 
	  * in this symbol table.  (If the debug string is accessed after
	  * this is called, then it will be recomputed.) */
	void freeAllDebugStrings();

private:
	// Look up a string in the table; add it if necessary.
	SymbolData* newRef(const wchar_t *str);
	SymbolData* newRef(const wchar_t* str, size_t off, size_t len);
	// Delete all memory owned by a given SymbolData.  Requires: data->ref_count==0.
	void delRef(SymbolData *data);
	// Pointer to the implementation: contains the data structures actually used
	// to implement the SymbolTable.
	SymbolTableImpl *_impl;
	// Pointer to the SymbolData for the NULL symbol.  This special
	// symbol never compares equal to any string-valued symbol, and
	// its reference count will never drop to zero.  Its str is NULL.
	SymbolData *_nullSymbol;
};

/*#################################################################################*/
/** An interned string; i.e., a thin wrapper for a string where we guarantee that 
  * exactly one Symbol is defined for each string value.  This allows the Symbols to 
  * be compared using simple (and fast) pointer identity comparisons.  It also saves 
  * memory by preventing us from keeping multiple copies of the same string.  For 
  * more information on string interning, see:
  * <http://en.wikipedia.org/wiki/String_interning> */
class SERIF_EXPORTED Symbol {
public:
	//======================= Symbol Tables ==============================

	/** Return the default global static SymbolTable.  This is the SymbolTable 
	  * that is used by the Symbol constructors when an explicit SymbolTable is
	  * not specified*/
	static SymbolTable *defaultSymbolTable();

	//======================= Constructors ===============================

	/** Return the NULL Symbol. This symbol is distinct from all string symbols. */
	Symbol(): data(defaultSymbolTable()->_nullSymbol) { SYMBOL_DATA_INCREF(data); }
	explicit Symbol(SymbolTable *table): data(table->_nullSymbol) { SYMBOL_DATA_INCREF(data); }

	/** Return the symbol for the given string. */
	Symbol(const wchar_t* s): data(defaultSymbolTable()->newRef(s)) {}
	Symbol(const wchar_t* s, SymbolTable *table): data(table->newRef(s)) {}

	/** Return the symbol for the given string. */
	Symbol(const std::wstring & s) : data(defaultSymbolTable()->newRef(s.c_str())) {}
	Symbol(const std::wstring & s, SymbolTable *table) : data(table->newRef(s.c_str())) {}

	/** Return the symbol for a substring of the given string, starting at 
	  * `s+off`, and with a length of `len`. */
	Symbol(const wchar_t *s, size_t off, size_t len): data(defaultSymbolTable()->newRef(s, off, len)) {}
	Symbol(const wchar_t *s, size_t off, size_t len, SymbolTable *table): data(table->newRef(s, off, len)) {}

	//======================== Instance Methods ==========================

	// Copy constructor and destructor.
	Symbol(const Symbol &symbol): data(symbol.data) { SYMBOL_DATA_INCREF(data); }
	~Symbol() { SYMBOL_DATA_DECREF(data); }

	// Assignment operator.
	Symbol& operator=(const Symbol &s) {
		#ifdef SYMBOL_REF_COUNT
			SYMBOL_DATA_INCREF(s.data);
			SYMBOL_DATA_DECREF(data); 
		#endif
		data = s.data;
		return *this;
	}
	
	/** Return true if this is the NULL symbol. */
	bool is_null() const { return data->str == NULL; }

	/** Return a hash code for this Symbol. */
	size_t hash_code() const             { return data->hash_value; }

	/** Return the string contents of this Symbol. */
	wchar_t const * to_string() const    { return data->str; }

	/** Return an ASCII transliteration of this Symbol.  This transliteration may
	  * be truncated or mangled, and should only be used for debugging purposes. */
	char const * to_debug_string() const;

	// Comparison operators.
    bool operator== (const Symbol &sym2) const {return data == sym2.data;}
    bool operator!= (const Symbol &sym2) const {return data != sym2.data;}
	bool operator< (const Symbol &sym2) const {
		if (sym2.data->str==0) return 0; // No symbol is less than NULL.
		if (data->str==0) return 1; // NULL is less than all symbols except itself.
		return (wcscmp(data->str, sym2.data->str) < 0);
	}

	// String operators
	const Symbol operator+ (const Symbol &sym2) const;

	//========================== Associated Types ========================

	/** A fast ordering operator for inner loops needing a fast total ordering over Symbols. */
	struct fast_less_than {
		inline bool operator()( const Symbol & l, const Symbol & r ) const {
			return l.data < r.data; }};

	// Define types Symbol::HashSet, Symbol::HashMap<V>, and Symbol::SymbolGroup.
	struct Hash {size_t operator()(const Symbol& s) const {return s.hash_code();}};
	struct Eq {bool operator()(const Symbol& s1, const Symbol& s2) const {return s1 == s2;}};
	typedef hash_set<Symbol, Hash, Eq> HashSet;
	template<typename ValueT> class HashMap: public serif::hash_map<Symbol, ValueT, Hash, Eq> {};
	typedef std::set<Symbol, Symbol::fast_less_than> SymbolGroup;
	
	//=========================== Static Methods =========================

	/** Return true if the given string has been interned. */
	static bool member(const wchar_t* str) {
		return defaultSymbolTable()->contains(str); }

	/** Remove any symbols whose ref_count is zero from the set of interned strings. 
	  * Even when SYMBOL_THREADSAFE is declared, this method is still not thread
	  * safe; only call it if you are the only thread! */
	static size_t discardUnusedSymbols(bool verbose=false) {
		return defaultSymbolTable()->discardUnusedSymbols(verbose); }

	/** Initialize the Symbol table with common words, read from the file
	  * ParamReader::getParam("symbol_table_initialization_file").  This will
	  * permanantly intern all strings contained in that file (i.e., their 
	  * reference count will never go to zero, so they will not be deleted by 
	  * discardUnusedSymbols). */
	static void initializeSymbolsFromFile() {
		defaultSymbolTable()->initializeSymbolsFromFile(); }

	/** Return the hash code for the given string. */
    static size_t hash_str(const wchar_t* s);
	/** Run unit tests and display the results.  Return true if successful. */
	static bool unitTest();
	// Used for determining whether a Symbol is contained in a predefined set of Symbol objects.
	// See notes in NewSymbol.cpp.
	static SymbolGroup makeSymbolGroup(const std::wstring & space_separated_words);
	//================== Related non-static method =======================
	bool isInSymbolGroup(const SymbolGroup & sset) const {
		return (sset.find(*this) != sset.end());
	}
private:
	//=========================== Private ================================
	SymbolData *data;

};

SERIF_EXPORTED std::ostream& operator<<(std::ostream&, const Symbol &);
SERIF_EXPORTED std::wostream& operator<<(std::wostream&, const Symbol &);
class UTF8OutputStream;
SERIF_EXPORTED UTF8OutputStream& operator<<(UTF8OutputStream&, const Symbol&);

// allows Symbol to be directly used by std::hash_map && std::hash_set
#if !defined(_WIN32)
#if defined(__APPLE_CC__)
namespace std {
#else
#include <ext/hash_map>
namespace __gnu_cxx {
#endif
	template<> struct hash<Symbol> {
        size_t operator()( const Symbol & s ) const { return s.hash_code(); };
	};
}
#endif

inline size_t hash_value( const Symbol & s ){ return s.hash_code(); }

typedef serif::hash_map<int, Symbol, serif::IntegerHashKey, serif::IntegerEqualKey> IntToSymbolMap;


//======================== NGrams ===============================
// An NGram is a fixed-length array of Symbols.  They are used in
// several tight loops in NgramScoreTable.h and Cache.h, so we take
// some extra care to define optimized copy/hash/equality operations
// for them.

/** Return true if the given ngrams contain corresponding symbols.
 * I.e., s1[i]==s2[i] for 0<=i<N. */
template<size_t N>
struct NGramEquals {
	bool operator()(const Symbol* s1, const Symbol* s2) const {
		for (size_t i = 0; i < N; i++) {
            if (s1[i] != s2[i]) 
				return false;
		}
		return true;
	}
};

/** Return a hash value for the given ngram.  This hash function is
 * slower than NGramQuickHash(), but will generate fewer
 * collisions. */
template<size_t N>
struct NGramHash {
	size_t operator()(const Symbol* s) const {
		return boost::hash_range(s, s+N);
	}
};

/** Return a hash value for the given ngram.  This hash function is
 * very fast, but will generate more collisions than NGramHash(). */
template<size_t N>
struct NGramQuickHash {
	size_t operator()(const Symbol* s) const {
		size_t val = s[0].hash_code();
		for (size_t i = 1; i < N; i++)
			val = (val << 2) + s[i].hash_code();
		return val;
	}
};
	
//========================= Loop Unrolling =============================
// the following template specializations do loop unrolling for
// NGramHash, NGramEquals, and NGramQuickHash, since these are used in
// several tight loops where where this actually makes a difference
// (based on profiling).
template<>
struct NGramHash<8> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		boost::hash_combine(result, *(++s)); // s[3]
		boost::hash_combine(result, *(++s)); // s[4]
		boost::hash_combine(result, *(++s)); // s[5]
		boost::hash_combine(result, *(++s)); // s[6]
		boost::hash_combine(result, *(++s)); // s[7]
		return result;
	}
};
template<>
struct NGramHash<7> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		boost::hash_combine(result, *(++s)); // s[3]
		boost::hash_combine(result, *(++s)); // s[4]
		boost::hash_combine(result, *(++s)); // s[5]
		boost::hash_combine(result, *(++s)); // s[6]
		return result;
	}
};
template<>
struct NGramHash<6> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		boost::hash_combine(result, *(++s)); // s[3]
		boost::hash_combine(result, *(++s)); // s[4]
		boost::hash_combine(result, *(++s)); // s[5]
		return result;
	}
};
template<>
struct NGramHash<5> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		boost::hash_combine(result, *(++s)); // s[3]
		boost::hash_combine(result, *(++s)); // s[4]
		return result;
	}
};
template<>
struct NGramHash<4> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		boost::hash_combine(result, *(++s)); // s[3]
		return result;
	}
};
template<>
struct NGramHash<3> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		boost::hash_combine(result, *(++s)); // s[2]
		return result;
	}
};
template<>
struct NGramHash<2> {
	size_t operator()(const Symbol* s) const {
		size_t result = 0;
		boost::hash_combine(result, *s);     // s[0]
		boost::hash_combine(result, *(++s)); // s[1]
		return result;
	}
};

template<>
struct NGramEquals<8> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]) && (s1[3] == s2[3]) &&
				 (s1[4] == s2[4]) && (s1[5] == s2[5]) &&
				 (s1[6] == s2[6]) && (s1[7] == s2[7]));
	}
};

template<>
struct NGramEquals<7> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]) && (s1[3] == s2[3]) &&
				 (s1[4] == s2[4]) && (s1[5] == s2[5]) &&
				 (s1[6] == s2[6]));
	}
};

template<>
struct NGramEquals<6> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]) && (s1[3] == s2[3]) &&
				 (s1[4] == s2[4]) && (s1[5] == s2[5]));
	}
};

template<>
struct NGramEquals<5> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]) && (s1[3] == s2[3]) &&
				 (s1[4] == s2[4]));
	}
};

template<>
struct NGramEquals<4> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]) && (s1[3] == s2[3]));
	}
};

template<>
struct NGramEquals<3> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]) &&
				 (s1[2] == s2[2]));
	}
};

template<>
struct NGramEquals<2> {
	bool operator()(const Symbol *s1, const Symbol *s2) const {
		return ( (s1[0] == s2[0]) && (s1[1] == s2[1]));
	}
};

template<>
struct NGramQuickHash<8> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<<14) +
				 ((s[1].hash_code())<<12) +
				 ((s[2].hash_code())<<10) +
				 ((s[3].hash_code())<< 8) +
				 ((s[4].hash_code())<< 6) +
				 ((s[5].hash_code())<< 4) +
				 ((s[6].hash_code())<< 2) +
				 ((s[7].hash_code())<< 0));

	}
};
template<>
struct NGramQuickHash<7> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<<12) +
				 ((s[1].hash_code())<<10) +
				 ((s[2].hash_code())<< 8) +
				 ((s[3].hash_code())<< 6) +
				 ((s[4].hash_code())<< 4) +
				 ((s[5].hash_code())<< 2) +
				 ((s[6].hash_code())<< 0));
	}
};

template<>
struct NGramQuickHash<6> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<<10) +
				 ((s[1].hash_code())<< 8) +
				 ((s[2].hash_code())<< 6) +
				 ((s[3].hash_code())<< 4) +
				 ((s[4].hash_code())<< 2) +
				 ((s[5].hash_code())<< 0));
	}
};

template<>
struct NGramQuickHash<5> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<< 8) +
				 ((s[1].hash_code())<< 6) +
				 ((s[2].hash_code())<< 4) +
				 ((s[3].hash_code())<< 2) +
				 ((s[4].hash_code())<< 0));
	}
};

template<>
struct NGramQuickHash<4> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<< 6) +
				 ((s[1].hash_code())<< 4) +
				 ((s[2].hash_code())<< 2) +
				 ((s[3].hash_code())<< 0));
	}
};

template<>
struct NGramQuickHash<3> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<< 4) +
				 ((s[1].hash_code())<< 2) +
				 ((s[2].hash_code())<< 0));
	}
};

template<>
struct NGramQuickHash<2> {
	size_t operator()(const Symbol * s) const {
		return ( ((s[0].hash_code())<< 2) +
				 ((s[1].hash_code())<< 0));
	}
};


#endif
