// Copyright 2010 by BBN Technologies Corp.
// All Rights Reserved.

#ifdef _WIN32
#include <limits.h>
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#endif

#include <boost/pool/pool.hpp>
#include "Generic/common/leak_detection.h" // This must be the first #include -- but not before pool.

#include "dynamic_includes/common/SymbolDefinitions.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/StringTransliterator.h"
#include "Generic/common/limits.h"

// for file initialization
#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"

#include <vector>
#include <iomanip>
#include <assert.h>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/algorithm/string/split.hpp>                       
#include <boost/algorithm/string/classification.hpp>    
#include <limits.h>
#include <boost/scoped_ptr.hpp>
#include <boost/math/common_factor_ct.hpp>

/** The maximum number of chars in the debug version of a symbol.  This
 * is just used for a single static character array, so setting it higher
 * would have a negligable effect on memory usage. */
#define MAX_DEBUG_SYMBOL_CHARS 1023

// For debugging:
//#define PROFILE_SYMBOL

//======================================================================
// Locking & Thread safety

namespace {
#ifdef SYMBOL_THREADSAFE
	static boost::uint32_t spinlock = 0;
	inline void ACQUIRE_SYMBOL_LOCK() {
#if BOOST_VERSION < 104800
		while(boost::interprocess::detail::atomic_cas32(&spinlock, 1, 0));
#else
		while(boost::interprocess::ipcdetail::atomic_cas32(&spinlock, 1, 0));
#endif // BOOST_VERSION < 104800
	}
	inline void RELEASE_SYMBOL_LOCK() {
		assert(spinlock);
		spinlock = 0;
	}
#elif defined(SYMBOL_TEST_THREADSAFE)
	static boost::uint32_t spinlock = 0;
	inline void ACQUIRE_SYMBOL_LOCK() {
		if (spinlock != 0) {
			throw InternalInconsistencyException(
				"Symbol::AQUIRE_SYMBOL_LOCK (Symbol.cpp)",
				"Symbol's spinlock is already locked, while SYMBOL_TEST_THREADSAFE is defined.  (Do not use this flag if you're actually running multi-threaded!");
		}
		spinlock = 1;
	}
	inline void RELEASE_SYMBOL_LOCK() {
		if (spinlock != 1) {
			throw InternalInconsistencyException(
				"Symbol::RELEASE_SYMBOL_LOCK (Symbol.cpp)",
				"Symbol's spinlock is already unlocked, while SYMBOL_TEST_THREADSAFE is defined.  (Do not use this flag if you're actually running multi-threaded!");
		}
		spinlock = 0;
	}

#else
	inline void ACQUIRE_SYMBOL_LOCK() {}
	inline void RELEASE_SYMBOL_LOCK() {}
#endif
}

//======================================================================
// Memory Pools

struct FixedBlockSizePoolAllocator
{
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  static char * malloc(const size_type bytes)
  { return _new char[bytes]; }
  static void free(char * const block)
  { delete [] block; }
};

// The boost pool implementation doubles the block size it uses each
// time it needs to allocate a new block.  In our case, that results
// in very large blocks (much larger than necessary), which cause
// memory fragmentation.  This specialized sublcass simply ensures that
// a fixed block size is used in the pool (by resetting the protected
// variable 'next_size' each time malloc is called).
class FixedBlockSizePool: private boost::pool<FixedBlockSizePoolAllocator> {
private:
	size_t items_per_block;
public:
	explicit FixedBlockSizePool(size_type item_size, size_type items_per_block):
			items_per_block(items_per_block), 
				boost::pool<FixedBlockSizePoolAllocator>(item_size, items_per_block) {}
	void *malloc() {
		next_size = items_per_block;
		#ifdef SYMBOL_USE_ORDERED_POOL
			return boost::pool<FixedBlockSizePoolAllocator>::ordered_malloc();
		#else
			return boost::pool<FixedBlockSizePoolAllocator>::malloc();
		#endif
	}
	void free(void * const chunk) {
		#ifdef SYMBOL_USE_ORDERED_POOL
			boost::pool<FixedBlockSizePoolAllocator>::ordered_free(chunk); 
		#else
			boost::pool<FixedBlockSizePoolAllocator>::free(chunk); 
		#endif
	}
	bool purge_memory() {
		return boost::pool<FixedBlockSizePoolAllocator>::purge_memory();
	}
	bool release_memory() {
		// Note: release_memory() is only available if we use an ordered
		// pool; but doing so requires that we use ordered_free(), which
		// is O(N) with respect to the size of the free list!
		#ifdef SYMBOL_USE_ORDERED_POOL
			return boost::pool<FixedBlockSizePoolAllocator>::release_memory();
		#else
			return false;
		#endif
	}
	size_t getMemoryUsage() {
		size_t num_blocks = 0;
		for (boost::details::PODptr<size_type> iter = list; iter.valid(); iter=iter.next()) {
			++num_blocks;
		}
		const size_type partition_size = alloc_size();
		const size_type POD_size = next_size * partition_size +
            boost::math::static_lcm<sizeof(size_type), sizeof(void *)>::value + sizeof(size_type);
		return num_blocks * POD_size;
	}
};

//======================================================================
// StringPool

/** A StringPool is a collection of boost memory pools used to store the 
  * string contents of Symbol objects.  A separate subpool is created 
  * for each string length, from 0 to SYMBOL_MAX_STRING_POOL_STRINGLEN (inclusive).   
  * Strings that are longer than SYMBOL_MAX_STRING_POOL_STRINGLEN are allocated  
  * and freed using new and delete. */
template<typename CharT>
class StringPool {
private:
	std::vector<FixedBlockSizePool *> subPools;
	static const size_t NUM_SUBPOOLS = SYMBOL_MAX_STRING_POOL_STRINGLEN+1;
	std::string name; // For debugging.
	size_t stringLength(const CharT* s) {
		size_t len=0;
		while (*(s++)) ++len;
		return len;
	}
public:
	StringPool(std::string const &name): name(name) {
		for (size_t i=0; i<NUM_SUBPOOLS; ++i) {
			size_t str_size = (i+1)*sizeof(CharT); // incl space for null terminator.
			subPools.push_back(_new FixedBlockSizePool(str_size, SYMBOL_STRING_BLOCK_SIZE/str_size));
		}
	}
	~StringPool() {
		for (size_t i=0; i<subPools.size(); ++i) {
			delete subPools[i];
		}
	}
	const CharT* copy(const CharT* src) {
		// Allocate space for the copied string.
		CharT *result;
		size_t len=stringLength(src);
		if (len < NUM_SUBPOOLS) {
			FixedBlockSizePool *pool = subPools[len];
			result = static_cast<CharT*>(pool->malloc());
		} else {
			result = _new CharT[len+1]; // incl space for null terminator.
		}
		// Copy the string and add a null terminator.
		CharT *dst = result;
		while (*src) {*(dst++) = *(src++);}
		*dst = 0;

		return result;
	}
	void free(const CharT* str) {
		// Delete it using the same method we used to allocate it.
		size_t len=stringLength(str);
		if (len < NUM_SUBPOOLS) {
			subPools[len]->free(const_cast<CharT*>(str));
		} else {
			delete[] str;
		}
	}
	/** Purge memory from the associated pools.  This does *not* purge memory for
	  * strings that are larger than SYMBOL_MAX_STRING_POOL_STRINGLEN. */
	bool purge_memory() {
		bool any_released = false;
		for (size_t i=0; i<NUM_SUBPOOLS; ++i) {
			any_released |= subPools[i]->purge_memory();
		}		
		return any_released;
	}
	/** Release memory from the associated pools.  This does *not* purge memory for
	  * strings that are larger than SYMBOL_MAX_STRING_POOL_STRINGLEN. */
	bool release_memory() {
		bool any_released = false;
		for (size_t i=0; i<NUM_SUBPOOLS; ++i) {
			any_released |= subPools[i]->release_memory();
		}
		return any_released;
	}
	size_t getMemoryUsage() {
		size_t memory_usage = 0;
		for (size_t i=0; i<NUM_SUBPOOLS; ++i) {
			memory_usage += subPools[i]->getMemoryUsage();
		}
		return memory_usage;
	}
};

StringPool<char>& debugStringPool() {
	// We intentionally never call the destructor for this.  
	static StringPool<char> pool("Symbol Debug String Pool");
	return pool;
}

//======================================================================
// SymbolTable

// Define the class Dictionary (a hash-set of pointers to the SymbolData
// objects).  Note that we never store the NULL Symbol in the symbol
// dictionary, so we don't have to worry about e->str being NULL.
struct EqualSymbolDataPtr {
	bool operator()(const SymbolData* e1, const SymbolData* e2) const
		{ return wcscmp(e1->str, e2->str) == 0; }};
struct HashSymbolDataPtr {
	size_t operator()(const SymbolData* e) const
		{ return e->hash_value; }};
typedef hash_set<SymbolData*, HashSymbolDataPtr, EqualSymbolDataPtr> Dictionary;

// This struct holds all the private members used by a SymbolTable.
// We keep it separate from the SymbolTable class itself because 
// otherwise we would need to #include <boost/pool> in the Symbol.h
// header file; and that has some indirect side-effects (such as
// definining preprocessor macros) that we'd rather keep contained --
// Symbol.h gets #included by almost every file in Serif.
class SymbolTableImpl: boost::noncopyable {
	friend class SymbolTable;
private:
	FixedBlockSizePool symbolDataPool;
	StringPool<wchar_t> stringPool;
	Dictionary dictionary; // Hash-set of pointers into symbolDataPool
	std::string tableName; // For debugging purposes only

	SymbolTableImpl(std::string const &tableName): 
		symbolDataPool(sizeof(SymbolData), SYMBOL_ENTRY_BLOCK_SIZE), 
		stringPool("Symbol String Pool"),
		dictionary(SYMBOL_TABLE_BUCKETS),
		tableName(tableName) {}
};

SymbolTable::SymbolTable(std::string const &tableName): 
_impl(_new SymbolTableImpl(tableName)) {
	// Create the null symbol.  We intentionally do not add it to 
	// the Symbol dictionary.
	_nullSymbol = static_cast<SymbolData*>(_impl->symbolDataPool.malloc());
	_nullSymbol->str = 0;
	_nullSymbol->hash_value = 0;
	_nullSymbol->debug_str = 0;
	_nullSymbol->ref_count = 1;
	// Always use the en_US.utf8 locale -- this defines the behavior
	// of functions like iswspace(), isupper() etc.  The main reason we
	// do this here (rather than somewhere else) is that we can be fairly
	// confident that all current SERIF code will initialize a symbol
	// table.  (In particular, we don't call this from main() because we
	// don't want some standalone trainer binary to forget to call this.)
	setlocale(LC_ALL, "en_US.utf8");
}

SymbolTable::~SymbolTable() { 
	ACQUIRE_SYMBOL_LOCK();
	delete _impl; 
	RELEASE_SYMBOL_LOCK();
}

size_t SymbolTable::size() {
	return _impl->dictionary.size()+1; // include the NULL symbol.
}

SymbolData* SymbolTable::newRef(const wchar_t *str) {
	// Check for NULL Symbol.
	if (str == NULL) { 
		SYMBOL_DATA_INCREF(_nullSymbol);
		return _nullSymbol; 
	}

	// Look up the given string in our dictionary.
	size_t hash_value = Symbol::hash_str(str);
	SymbolData querySymData;
	querySymData.str = str;
	querySymData.hash_value = hash_value;
	ACQUIRE_SYMBOL_LOCK();
	SymbolData *symData = _impl->dictionary.find_singleton(&querySymData, hash_value);

	// If it's not found, then we need to create a new symbol data object for it.  
	if (symData == NULL) {
		symData = static_cast<SymbolData*>(_impl->symbolDataPool.malloc());
		symData->str = _impl->stringPool.copy(str);
		symData->debug_str = 0;
		symData->hash_value = hash_value;
#ifdef SYMBOL_REF_COUNT
		symData->ref_count = 1;
#endif
		_impl->dictionary.insertWithoutChecking(symData);
#ifdef PROFILE_SYMBOL
		if ((size()%10000) == 0) {
			printDebugInfo();
		}
#endif
	} else {
		SYMBOL_DATA_INCREF(symData);
	}
	RELEASE_SYMBOL_LOCK();
#ifdef SYMBOL_REF_COUNT
	assert (symData->ref_count > 0);
#endif
	return symData;
}

SymbolData* SymbolTable::newRef(const wchar_t* str, size_t off, size_t len) {
	SymbolData *result;
	wchar_t *substring = _new wchar_t[len+1];
    wcsncpy(substring, str + off, len);
	substring[len] = L'\0';
	result = newRef(substring);
	delete[] substring;
	return result;
}

void SymbolTable::delRef(SymbolData *symData) {
	ACQUIRE_SYMBOL_LOCK();
	_impl->dictionary.erase(symData);
	_impl->stringPool.free(symData->str);
	if (symData->debug_str) { debugStringPool().free(symData->debug_str); }
	_impl->symbolDataPool.free(symData);
	RELEASE_SYMBOL_LOCK();
}

size_t SymbolTable::discardAllSymbols() {
	size_t num_symbols_discarded = 0;
	Dictionary::iterator iter = _impl->dictionary.begin();
	Dictionary::iterator dictEnd = _impl->dictionary.end();
	while(iter != dictEnd) {
		SymbolData* symData = *iter;
		++iter;
		++num_symbols_discarded;
		delRef(symData);
	}
	debugStringPool().release_memory();
	_impl->stringPool.purge_memory();
	_impl->symbolDataPool.purge_memory();
	return num_symbols_discarded;
}

size_t SymbolTable::discardUnusedSymbols(bool verbose) {
	size_t num_symbols_discarded = 0;
#ifdef SYMBOL_REF_COUNT
	Dictionary::iterator iter = _impl->dictionary.begin();
	Dictionary::iterator dictEnd = _impl->dictionary.end();
	while(iter != dictEnd) {
		SymbolData* symData = (*iter);
		// Increment the iterator now (rather than at the end of the loop); 
		// otherwise it will become invalid if/when we call delRef().
		++iter;
		if (symData->ref_count == 0) {
			if (verbose) {
				std::wcerr << "  Discard: [" << symData->str << "]" << std::endl;
			}
			assert(symData->str != 0); // NULL symbol should not be in the dictionary.
			delRef(symData);
			++num_symbols_discarded;
		} else {
			if (verbose) {
				std::wcerr << "     Keep: [" << symData->str << "] (" << symData->ref_count << ")" << std::endl;
			}
		}
	}
	debugStringPool().release_memory();
	_impl->stringPool.release_memory();
	if (verbose) {
		std::cerr << "Discarded " << num_symbols_discarded << " unused symbols!" << std::endl;
	}
	SessionLogger::info("symbol-table") 
		<< "Discarded " << num_symbols_discarded << " unused symbols.";
#endif
	return num_symbols_discarded;
}

bool SymbolTable::contains(const wchar_t* str)
{
	SymbolData symData;
	symData.str = str;
	symData.hash_value = Symbol::hash_str(str);
	ACQUIRE_SYMBOL_LOCK();
    bool result = (_impl->dictionary.find(&symData) != _impl->dictionary.end());
	RELEASE_SYMBOL_LOCK();
	return result;
}

void SymbolTable::initializeSymbolsFromFile() {
	//return;
	std::string symbol_file = ParamReader::getParam("symbol_table_initialization_file");
	if (!symbol_file.empty()) {
		std::cout << "Initializing Symbol Table...\n";
		boost::scoped_ptr<UTF8InputStream> input_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& input(*input_scoped_ptr);
		input.open(symbol_file.c_str());
		if (input.fail()) {
			throw UnexpectedInputException(
				"Symbol::initializeSymbolsForFile()",
				"Symbol initialization file not found -- "
				"check 'symbol_table_initialization_file' parameter...");
		}
		while (!input.eof()) {
			UTF8Token token;
			input >> token;
			newRef(token.chars());
		}
	}
}

// This method under-reports the actual memory usage, in a couple different
// ways.  First, for pooled memory, it doesn't include the portion of memory
// that is allocated but not yet used.  And second, it doesn't include the
// overhead associated with malloc.  But I doubt it's off by more than 5-10%.
void SymbolTable::printDebugInfo() {
	size_t num_symbols = 0;
	size_t num_zero_refcount_symbols = 0;

	// Memory usage stats:
	size_t mem_overhead = sizeof(SymbolTable) + sizeof(SymbolTableImpl);
	size_t mem_dictionary = _impl->dictionary.approximateSizeInBytes();
	size_t mem_symdata = 0;
	size_t mem_strings = 0;
	size_t mem_debug_strings = 0;
	size_t mem_zero_refcount = 0;
	
	Dictionary::iterator dictEnd = _impl->dictionary.end();
	for(Dictionary::iterator iter = _impl->dictionary.begin();iter != dictEnd; ++iter) {
		SymbolData* symData = (*iter);
		++num_symbols;
		mem_symdata += sizeof(SymbolData);
		if (symData->str)
			mem_strings += (wcslen(symData->str)+1)*sizeof(wchar_t);
		if (symData->debug_str)
			mem_debug_strings += (strlen(symData->debug_str)+1)*sizeof(char);
#ifdef SYMBOL_REF_COUNT
		if (symData->ref_count == 0) {
			++num_zero_refcount_symbols;
			mem_zero_refcount += sizeof(SymbolData);
			if (symData->str)
				mem_zero_refcount += (wcslen(symData->str)+1)*sizeof(wchar_t);
			if (symData->debug_str)
				mem_zero_refcount += (strlen(symData->debug_str)+1)*sizeof(char);
		}
#endif
	}

	// These are better estimates of the memory usages of the pools:
	mem_strings = _impl->stringPool.getMemoryUsage();
	mem_debug_strings = debugStringPool().getMemoryUsage();
	mem_symdata = _impl->symbolDataPool.getMemoryUsage();

	size_t mem_total = (mem_overhead + mem_dictionary + 
		mem_symdata + mem_strings + mem_debug_strings);

	std::streamsize orig_precision = std::cout.precision(2);
	std::ios_base::fmtflags orig_flags = std::cout.flags();
	std::cout.setf(std::ios::fixed);
	std::cout << "SymbolTable(\"" << _impl->tableName << "\")\n"
		<< "------------------------------------\n";
	if (size() < 10000) 
		std::cout << "  Number of Symbols:" << std::setw(10) << size() << "\n";
	else
		std::cout << "  Number of Symbols:" << std::setw(10) << size()/1000 << " k\n";
	if (size())
		std::cout << "  Memory/symbol:    " << std::setw(10) << (mem_total/size()) << " bytes\n";
	std::cout << "  Memory Usage:\n"
		<< "    Dictionary:     " << std::setw(10) << (mem_dictionary/1024.0/1024.0) << " MB\n"
		<< "    Strings:        " << std::setw(10) << (mem_strings/1024.0/1024.0) << " MB\n"
		<< "    Debug Strings:  " << std::setw(10) << (mem_debug_strings/1024.0/1024.0) << " MB\n"
		<< "    SymbolData:     " << std::setw(10) << (mem_symdata/1024.0/1024.0) << " MB\n"
		<< "                       ----------\n"
		<< "    Total:          " << std::setw(10) << (mem_total/1024.0/1024.0) << " MB\n"
		<< "  Freeable symbols:\n"
		<< "    # Freeable Syms:" << std::setw(10) << (num_zero_refcount_symbols) << "\n"
		<< "    Freeable Memory:" << std::setw(10) << (mem_zero_refcount/1024.0/1024.0) << " MB\n"
		<< "------------------------------------" << std::endl;
	std::cout.flags(orig_flags);
	std::cout.precision(orig_precision);
}

void SymbolTable::freeAllDebugStrings() {
	ACQUIRE_SYMBOL_LOCK();
	Dictionary::iterator iter = _impl->dictionary.begin();
	Dictionary::iterator dictEnd = _impl->dictionary.end();
	while(iter != dictEnd) {
		SymbolData* symData = *iter;
		if (symData->debug_str)
			debugStringPool().free(symData->debug_str);
		symData->debug_str = 0;
		++iter;
	}
	debugStringPool().release_memory();
	RELEASE_SYMBOL_LOCK();
}

namespace {
	SymbolTable *createDefaultSymbolTable() {
		#ifdef _WIN32 // Disable allocation tracking
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) &~ _CRTDBG_ALLOC_MEM_DF);
		#endif
		SymbolTable* _symbolTable = new SymbolTable("DefaultSymbolTable");
		#ifdef _WIN32 // Enable allocation tracking
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF);
		#endif
		return _symbolTable;
	}
}

SymbolTable* Symbol::defaultSymbolTable() {
	// We intentionally never call the destructor for this: there are static
	// symbols, and if we destroyed the symbol table before we destroyed the
	// symbols, then we'd be in trouble.
	static SymbolTable* _symbolTable = _new SymbolTable("DefaultSymbolTable");
	//static SymbolTable* _symbolTable = createDefaultSymbolTable();
	return _symbolTable;
}

//======================================================================
// Symbol I/O

std::ostream& operator<<(std::ostream& os, const Symbol& symbol) {
	if (symbol.is_null())
		throw InternalInconsistencyException("NewSymbol::operator<<",
			"Attempt to print an empty symbol");
	os << symbol.to_debug_string();
	return os;
}

std::wostream& operator<<(std::wostream& os, const Symbol& symbol) {
	if (symbol.is_null())
		throw InternalInconsistencyException("NewSymbol::operator<<",
			"Attempt to print an empty symbol");
	os << symbol.to_string();
	return os;
}

UTF8OutputStream& operator<<(UTF8OutputStream& os, const Symbol& symbol) {
	if (symbol.is_null())
		throw InternalInconsistencyException("NewSymbol::operator<<",
			"Attempt to print an empty symbol");
	os << symbol.to_string();
	return os;
}

//======================================================================
// Other Symbol methods

// The constants used by the FNV-1a hash algorithm depend on the size
// of the variable used to hold the hash (i.e., the size of size_t).
#if SIZE_MAX==0xffffffff
#define FNV_HASH_PRIME     0x01000193u
#define FNV_OFFSET_BASIS   0x811c9dc5u
#else
#define FNV_HASH_PRIME     0x00000100000001b3u
#define FNV_OFFSET_BASIS   0xcbf29ce484222325u
#endif
#define USE_FNV_HASH

size_t Symbol::hash_str(const wchar_t* s)
{
#ifdef USE_FNV_HASH
	// Use the public-domain "FNV-1a" hash algorithm:
	// <http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash>
	// Note: we only include the first 2 bytes of each wchar_t.
	// (wchar_t may be 16-bit or 32-bit, depending on the platform)
	size_t result = FNV_OFFSET_BASIS;
	if (!s)
		return result;
	while (*s != L'\0') {
		result ^= (*s & 0xff);
		result *= FNV_HASH_PRIME;
		result ^= (*s++ >> 8);
		result *= FNV_HASH_PRIME;
	}
	return result;

#else

	if (s==0 || *s == L'\0')
		return 0;

	size_t result = *s++;
	while (*s != L'\0') {
		result = ((result << LEFT_BIT_SHIFT) | (result >> (RIGHT_BIT_SHIFT))) ^ *s++;
	}
	return result;

#endif
}

const Symbol Symbol::operator+ (const Symbol &sym2) const {
	// Allocate a temporary buffer for the combined Symbol data
	size_t len = is_null() ? 0 : wcslen(data->str);
	size_t len2 = sym2.is_null() ? 0 : wcslen(sym2.data->str);
	if (len + len2 == 0)
		return Symbol();
	wchar_t* concat = _new wchar_t[len + len2 + 1];
	concat[len + len2] = L'\0';
	if (len > 0)
		wcscpy(concat, data->str);
	if (len2 > 0)
		wcscpy(&(concat[len]), sym2.data->str);
	Symbol concatSym = Symbol(concat);
	delete[] concat;
	return concatSym;
}

char const * Symbol::to_debug_string() const {
	if (data == NULL) return 0;
	if ((data->debug_str == NULL) && (data->str != NULL)) {
		ACQUIRE_SYMBOL_LOCK();
	    static char debug_s[MAX_DEBUG_SYMBOL_CHARS+1];
		StringTransliterator::transliterateToEnglish(debug_s, data->str, MAX_DEBUG_SYMBOL_CHARS);
		data->debug_str = debugStringPool().copy(debug_s);
		RELEASE_SYMBOL_LOCK();
	}
	return data->debug_str;
}

// The method makeSymbolGroup produces a set of symbols against which
// individual symbols can be tested for membership. This is both 
// more concise and more efficient than performing a long sequence 
// of equality tests. For example, rather than this:
//      if (sym == Symbol(L"court") || (sym == Symbol(L"courts")...)
// one could write the following function:
// static bool is_govt_type_symbol(const Symbol& sym) {
//    //Note: the SymbolGroup is statically defined, so it's only initialized once,
//	  //when the function is first called.
//    static Symbol::SymbolGroup govtSymbols = Symbol::makeSymbolGroup(
//				// Note: the C++ preprocessor will merge these strings, so make sure
//				// to leave a space at the end of each line except the last.
//				L"court courts army navy forces parliament "
//				L"legislature bureau agency agencies " 
//				L"ministry ministries department departments "
//				L"cabinet administration");
//        L"court courts army navy forces ");
//    return (sym.isInSymbolGroup(govtSymbols));
//}
// Note that the name "SymbolGroup" was used to avoid confusion with the 
// existing SymbolSet class (in SymbolSet.h).
// Although SymbolGroup is in the Symbol namespace and SymbolSet is not in
// a namespace, "Symbol::SymbolSet" and "SymbolSet" would be hard to distinguish.

Symbol::SymbolGroup Symbol::makeSymbolGroup(const std::wstring & space_separated_words) {
	std::vector<std::wstring> vec;
    boost::split(vec, space_separated_words, boost::is_any_of(L" "));
    return SymbolGroup(vec.begin(), vec.end());
}

//======================================================================
// Unit Test

// Macro used by the unit test method:
#define ASSERT(condition) \
	if (!(condition)) { \
		std::cerr << __FILE__ << ":" << __LINE__ << ": FAIL: " \
		<< (#condition) << std::endl; \
		return false; \
	} else { \
		std::cerr << __FILE__ << ":" << __LINE__ << ":   Ok: " \
		<< (#condition) << std::endl; }


bool Symbol::unitTest() {
#ifdef SYMBOL_REF_COUNT
	SymbolTable symbolTable("UnitTestSymbolTable");
	SymbolTable *table = &symbolTable;

	//ASSERT(table->_impl->dictionary.size() == 0);
	
	Symbol sym_hello(L"==hello==", table);
	Symbol sym_world(L"==world==", table);
	Symbol sym_hello2(L"==hello==", table);
	table->printDebugInfo();

	ASSERT(sym_hello == sym_hello2);
	ASSERT(sym_hello.data == sym_hello2.data);
	ASSERT(sym_world != sym_hello);
	ASSERT(sym_world.data != sym_hello.data);
	ASSERT(sym_hello.data->ref_count == 2);
	ASSERT(sym_world.data->ref_count == 1);
	{
		Symbol sym_hello3(L"==hello==", table);
		Symbol sym_Hello(L"==Hello==", table);
		ASSERT(sym_hello.data->ref_count == 3);
	}
	ASSERT(sym_hello.data->ref_count == 2);
	table->printDebugInfo();
	
	ASSERT(table->contains(L"==hello==") == true);
	sym_hello = Symbol();
	ASSERT(table->discardUnusedSymbols(false) == 1);
	ASSERT(table->contains(L"==hello==") == true);
	sym_hello2 = Symbol();
	ASSERT(table->contains(L"==hello==") == true);
	ASSERT(table->discardUnusedSymbols(false) == 1);
	table->printDebugInfo();
	ASSERT(table->contains(L"==hello==") == false);
	table->printDebugInfo();
#endif
	return true;
}
