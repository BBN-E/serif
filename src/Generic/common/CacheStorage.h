// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CACHE_STORAGE_H
#define CACHE_STORAGE_H

#include "Generic/common/Cache.h"

#include "Generic/common/ParamReader.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include <boost/scoped_ptr.hpp>


template<size_t N>
struct CacheStorage {

  static void readCache(const char* case_tag, const char* cacheSuffix, typename Cache<N>::simple& table, long max) {
    readCache(case_tag, cacheSuffix, 0, table, max);
  }

  static void writeCache(const char* case_tag, const char* cacheSuffix, typename Cache<N>::simple& table) {
    writeCache(case_tag, cacheSuffix, 0, table);
  }

  static void readCache(const char* case_tag, const char* cacheSuffix, const char* cache_tag, typename Cache<N>::simple& table, long max) {
	  std::string temp_buffer = ParamReader::getParam("probs_cache_in_path");
	  if(!temp_buffer.empty()) {
		  boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
		  UTF8InputStream& stream(*stream_scoped_ptr);
		  std::string buffer;
		  std::string case_tag_str(case_tag);
		  std::string cache_suffix_str(cacheSuffix);
		  if (cache_tag == 0) {
			  buffer = temp_buffer + case_tag_str + "-" + cache_suffix_str;
		  } else {
			  std::string cache_tag_str(cache_tag);			  
			  buffer = temp_buffer + cache_tag_str + "-" + case_tag_str + "-" + cache_suffix_str;
		  }
		  try {
			  stream.open(buffer.c_str());
			  init_table(stream, table, max);
			  stream.close();
		  } catch (UnexpectedInputException) {
			  SessionLogger::warn("cache") << "Unable to read cache pre-load file; skipping cache pre-load: " << buffer << std::endl;
			  return;
		  }
	  }
  }
  
  static void writeCache(const char* case_tag, const char* cacheSuffix, const char* cache_tag, typename Cache<N>::simple& table) {
	  std::string temp_buffer = ParamReader::getParam("probs_cache_out_path");
	  if(!temp_buffer.empty()) {
		  UTF8OutputStream stream;
		  std::string buffer;
		  std::string case_tag_str(case_tag);
		  std::string cache_suffix_str(cacheSuffix);
		  if (cache_tag == 0) {
			  buffer = temp_buffer + case_tag_str + "-" + cache_suffix_str;
		  } else {
			  std::string cache_tag_str(cache_tag);	
			  buffer = temp_buffer + cache_tag_str + "-" + case_tag_str + "-" + cache_suffix_str;
		  }
		  stream.open(buffer.c_str());
		  print_to_open_stream(stream, table);
		  stream.close();
	  }
  }

  private:
  
  static void print_to_open_stream(UTF8OutputStream& out, typename Cache<N>::simple& table) 
  {
    
    typename Cache<N>::sizeType table_size = table.size();
    out << (unsigned int)table_size;
    out << "\n";
    
    for (typename Cache<N>::simple::iterator iter = table.begin() ; iter != table.end() ; ++iter) {
      out << "((";
      for (size_t j = 0; j < N - 1; j++) {
        out << (*iter).first[j].to_string();
        out << " ";
      }
      out << (*iter).first[N - 1].to_string();
    
      out << ") ";
      out << (*iter).second;
      out << ")";
      out << "\n";
    }
    
  }
  
  static void init_table(UTF8InputStream& stream, typename Cache<N>::simple& table, long max)
  {
    
    UTF8Token token;
    
    int numEntries; 
    stream >> numEntries;
    if (numEntries > max) {
      numEntries = max;
    }
    
    std::cout << "Preloading " << numEntries << " entries into prob cache" << std::endl;
    
    for (int i = 0; i < numEntries; i++) {
      
      Symbol* ngram = _new Symbol[N];
      
      stream >> token;
      
      if (token.symValue() != SymbolConstants::leftParen) {
        char c[100];
        sprintf( c, "ERROR: ill-formed ngram record at entry: %d", i );
        throw UnexpectedInputException("CacheStorage::init_table()", c);
      }
      
      stream >> token;
      if (token.symValue() != SymbolConstants::leftParen) {
        char c[100];
        sprintf( c, "ERROR: ill-formed ngram record at entry: %d", i );
        throw UnexpectedInputException("CacheStorage::init_table()",c);
      }
      
      for (size_t j = 0; j < N; j++) {
        stream >> token;
        ngram[j] = token.symValue();
      }
      
      stream >> token;
      if (token.symValue() != SymbolConstants::rightParen) {
        char c[100];
        sprintf( c, "ERROR: ill-formed ngram record at entry: %d", i );
        throw UnexpectedInputException("CacheStorage::init_table()",c);
      }
      
      float score;
      stream >> score;
      
      stream >> token;
      if (token.symValue() != SymbolConstants::rightParen) {
        char c[100];
        sprintf( c, "ERROR: ill-formed ngram record at entry: %d", i );
        throw UnexpectedInputException("CacheStorage::init_table()",c);
      }
      
      table.insert(std::make_pair(ngram, score));
      
    }
  }

};

#endif
