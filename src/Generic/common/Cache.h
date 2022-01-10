// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CACHE_H
#define CACHE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/lru_cache.h"
#if defined(_WIN32) || defined(__APPLE_CC__)
enum CacheType { None, Simple };
#else 
enum CacheType { None,  Lru, Simple };
#endif

// Forward declaration
template<size_t N, typename HashMapClass> class NGramCache;
template<size_t N> struct CopyCacheNGram;

/** A template used to provide typedefs for simple and lru caches.
 * The caches themselves are implemented by NGramCache, defined below.
 *
 * Note: The decision to use NGramQuickHash rather than NGramHash is
 * justified based on profiling.  In particular, the quick hash runs
 * about 50 times faster than the "better" hash, but only causes about
 * a 50% speed penalty when looking through hash buckets.  Overall,
 * using NGramQuickHash gives about a 30% self-time speedup for the
 * xyzProbs::lookup() methods.
 */
template<size_t N>
struct Cache {
    typedef Symbol* NGRAM;
	typedef NGramQuickHash<N> HashKey;
	typedef NGramEquals<N> EqualKey;
	typedef size_t sizeType;

#if defined(_WIN32) || defined(__APPLE_CC__)
    typedef NGramCache<N, serif::hash_map<NGRAM, float, HashKey, EqualKey> > simple;
#else
    typedef NGramCache<N, __gnu_cxx::hash_map<NGRAM, float, HashKey, EqualKey> > simple;
    typedef NGramCache<N, lru_cache<NGRAM, float, HashKey, EqualKey> > lru;
#endif
};

/** A hash map from fixed-length Symbol arrays to float values.
 * Symbol arrays are compared and hashed based on the list of Symbol
 * values they contain.  When the cache needs to add a new entry,
 * it makes a private copy of the Symbol array, and it is responsible
 * for deleting that private copy when the SimpleCache is destroyed.
 */
template<size_t N, typename HashMapClass>
class NGramCache {
public:
	/** Create a new cache with the given maximum size.  */
	explicit NGramCache(size_t max_size=0) : _max_size(max_size), _cache(max_size) 
	{
		// _ngrams contains raw (unintialized) memory.  
		void *rawMemory = operator new[] (max_size * sizeof(Symbol) * N);
		_ngrams = static_cast<Symbol*>(rawMemory);
		// _ngext_ngram points to where the next ngram will go.
		_next_ngram = _ngrams;
	}

	~NGramCache() {
		// Destroy any ngrams we have constructed.
		for(Symbol *p=_ngrams; p!=_next_ngram; ++p)
			p->~Symbol();
		// Destroy the raw memory
		operator delete[](_ngrams);
	}

	#if defined(_WIN32)
		typedef typename HashMapClass::iterator iterator;
	#else
		typedef typename HashMapClass::const_iterator iterator;
	#endif

	typedef std::pair<Symbol*, float> value_type;

	/** Add a new entry to the cache, if there is room; otherwise, do
	 * nothing.  Precondition: ngram_key must not already be
	 * associated with a value in the cache. */
	void insert(value_type pair) {
		if (_cache.size() < _max_size) {
			// Make a local copy of the ngram key in the _ngrams vector.  This
			// also increments _next_ngram.
			Symbol *ngram_copy = _next_ngram;
			CopyCacheNGram<N>()(_next_ngram, pair.first);
			// Add the pair to the cache.
			_cache.insert(std::make_pair(ngram_copy, pair.second));
		}
	}

	iterator find(Symbol* ngram_key) {
		return _cache.find(ngram_key);
	}

	iterator begin() {
		return _cache.begin();
	}

	iterator end() {
		return _cache.end();
	}

	size_t size() {
		return _cache.size();
	}

	void clear() {
		// Clear the hash table.
		_cache.clear();
		// Destroy any ngrams we have constructed.
		for(Symbol *p=_ngrams; p!=_next_ngram; ++p)
			p->~Symbol();
		// Reset the _next_ngram pointer to the start of _ngrams.
		_next_ngram = _ngrams;
	}

private:
	const size_t _max_size;
	Symbol* _ngrams;
	Symbol* _next_ngram;
	HashMapClass _cache;
};


// Copy-construct the ngram at src into to dst.
template<size_t N>
struct CopyCacheNGram {
	void operator()(Symbol* &dst, Symbol* src) {
		for (size_t i=0; i<N; ++i)
			new(dst++) Symbol(*src++);
	}
};

// Loop unrolling for common values of N:
template<> struct CopyCacheNGram<8> {
	void operator()(Symbol* &dst, Symbol* src) { 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src); 
	}};
template<> struct CopyCacheNGram<7> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++);
		new(dst++) Symbol(*src);
	}};
template<> struct CopyCacheNGram<6> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src);
	}};
template<> struct CopyCacheNGram<5> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++);
		new(dst++) Symbol(*src);
	}};
template<> struct CopyCacheNGram<4> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src); 
	}};
template<> struct CopyCacheNGram<3> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src++); 
		new(dst++) Symbol(*src);
	}};
template<> struct CopyCacheNGram<2> {
	void operator()(Symbol* &dst, Symbol* src) {
		new(dst++) Symbol(*src++); new(dst++) Symbol(*src); 
	}};


#endif
