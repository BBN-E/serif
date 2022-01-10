// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NGRAM_SCORE_TABLE_H
#define NGRAM_SCORE_TABLE_H

#include <cstddef>

#if defined(_WIN32) || defined(__APPLE_CC__)
#include "Generic/common/hash_map.h"
#else
#include <ext/hash_map>
#endif

#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/Assert.h"

#include <boost/functional/hash.hpp>

#define USE_BOOST_HASH

using namespace std;

template <size_t NN>
struct HashKey {
  size_t operator()(const Symbol* s) const {
	  return NGramHash<NN>()(s);
  }
  HashKey(size_t n) {}
  HashKey() {}
};

template <>
struct HashKey<0> {
  size_t N_flex;
  size_t operator()(const Symbol* s) const {
#ifdef USE_BOOST_HASH
      return boost::hash_range(s, s+N_flex);
#else
    size_t val = s[0].hash_code();
    for (size_t i = 1; i < N_flex; i++)
      val = (val << 2) + s[i].hash_code();
    return val;
#endif
  }
  HashKey<0>(size_t n) : N_flex(n) {}
  HashKey<0>() : N_flex(0) {}
};

template <size_t NN>
struct EqualKey {
  bool operator()(const Symbol* s1, const Symbol* s2) const {
	  return NGramEquals<NN>()(s1, s2);
  }
  EqualKey(size_t n) {}
  EqualKey() {}
};

template <>
struct EqualKey<0> {
  size_t N_flex;
  bool operator()(const Symbol* s1, const Symbol* s2) const {
    for (size_t i = 0; i < N_flex; i++) {
      if (s1[i] != s2[i]) {
        return false;
      }
    }
    return true;
  }
  EqualKey<0>(size_t n) : N_flex(n) { }
  EqualKey<0>() : N_flex(0) { }
};

template <size_t N = 0>
class NgramScoreTableGen {
private:
    static const float targetLoadingFactor;
    const size_t N_flexible;

    HashKey<N> hasher;
    EqualKey<N> eqTester;

public:
#if defined(_WIN32) || defined(__APPLE_CC__)
    typedef typename serif::hash_map<Symbol*, float, HashKey<N>, EqualKey<N> > Table;
#else
    typedef typename __gnu_cxx::hash_map<Symbol*, float, HashKey<N>, EqualKey<N> > Table;
#endif
private:
    int numEntries;
    int numBuckets;
    Table table;
    int size;

public:
    NgramScoreTableGen(size_t n, UTF8InputStream& stream);
    NgramScoreTableGen(size_t n, int init_size);
    NgramScoreTableGen(UTF8InputStream& stream);
    NgramScoreTableGen(int init_size);
    ~NgramScoreTableGen();

    void print(const char *filename);
    void print_to_open_stream(UTF8OutputStream& out);

    inline float lookup(Symbol* ngram) const {
#if defined(_WIN32)
      typename Table::iterator iter = table.find(ngram);
#else
      typename Table::const_iterator iter = table.find(ngram);
#endif
      if (iter == table.end()) {
        return 0;
      }
      return (*iter).second;
    }

    int get_size() { return size; }
    void add(Symbol* ngram);
    void add(Symbol* ngram, float value);
    NgramScoreTableGen<N>* prune(int threshold);
    void reset();

    typename Table::iterator get_start() { return table.begin(); }
    typename Table::iterator get_end() { return table.end(); }
    typename Table::iterator get_element (Symbol* ngram) { return table.find(ngram); }
 private:
    int get_num_entries(UTF8InputStream& stream); 
    int get_num_buckets(int init_size);

};

typedef NgramScoreTableGen<> NgramScoreTable;

// Compile-time error checker
// From "Modern C++ Design", Alexandrescu
// We're using it to make sure that N is 0 or not 0 (as appropriate) in the
// correct places in the constructors

// If you get an error involving this, it's a compile-time error in
// whatever is calling NST_STATIC_CHECK
template<bool> struct CompileTimeError;
template<> struct CompileTimeError<true> {};
#define NST_COMPILE_CHECK(expr) (CompileTimeError<(expr) != 0>())

#endif
