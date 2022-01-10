// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

// Modified from Cube2/lib/nemisis/utilizing/lru_cache.hpp

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#if !defined(_WIN32) && !defined(__APPLE_CC__)
// Not currently supported for Windows or OS X
//  To port to Windows, add #if conditional for Generic/common/hash_map
//  and add value_type to Generic/common/hash_map
#include <ext/hash_map>

#include <list>
#include <utility>
#include <functional>  // for std::equal_to<T>
#include <cassert>

#include <boost/operators.hpp>

// 
// Implements an LRU cache that looks like an SGI STL hash_map.
// It supports a subset of the hash_map functions:
//
//   insert()
//   find()
//   operator[]
//

template<class KEY, class DATA, 
         class HASH_FUN, 
         class EQ_KEY >
  class lru_cache : public boost::equality_comparable<lru_cache<KEY, DATA, HASH_FUN, EQ_KEY> >{
  
  // Linked list of the stuff we would 
  // have stored in a hash_map. 
  // This is our actual data storage
  
  typedef std::list<typename __gnu_cxx::hash_map<KEY, DATA>::value_type> ListType;
    
  // Hash table of pairs:
  //   pair.first  : Iterator to data in the linked list
  //   pair.second : Hit-count (mass) of the element. 
  //                 How many times was the element asked for.
  // This is our index into the data storage.
  
  typedef std::pair<typename ListType::iterator, std::size_t> IndexElementType;
  typedef __gnu_cxx::hash_map<KEY, IndexElementType, HASH_FUN, EQ_KEY> IndexType;
  
  public:

  typedef KEY  key_type;
  typedef DATA data_type;
  
  typedef typename ListType::value_type       value_type;
  typedef typename ListType::iterator         iterator;
  typedef typename ListType::const_iterator   const_iterator;
  
  ///////////////////////////////////////////////////////////
  // constructors / destructor
  ///////////////////////////////////////////////////////////
  lru_cache() :
    max_elem(1024),
    num_elem(0u),
    element_list(),
    element_index() {}
  
  lru_cache(const lru_cache& rhs) :
    max_elem(rhs.max_elem),
    num_elem(rhs.num_elem),
    element_list(rhs.element_list),
    element_index(rhs.element_index) 
    {}
  
  lru_cache(std::size_t max_elem) : 
    max_elem(max_elem),
    num_elem(0u),
    element_list(),
    element_index()
    {}
  
  ~lru_cache() {}
  
  ///////////////////////////////////////////////////////////
  // copy value semantics
  ///////////////////////////////////////////////////////////
  
  void swap(lru_cache& rhs)
    {
      std::swap(max_elem, rhs.max_elem);
      std::swap(num_elem, rhs.num_elem);
      
      element_list.swap(rhs.element_list);
      element_index.swap(rhs.element_index);
    }
  
  lru_cache& operator=(lru_cache rhs)
    {
      swap(rhs);
      return *this;
    }
  
  ///////////////////////////////////////////////////////////
  // find an element in the cache
  // if found, move to the front of the list
  ///////////////////////////////////////////////////////////
  iterator find(const key_type& k)
    {
      assert(num_elem <= max_elem);
      
      // find element in index
      typename IndexType::iterator it = element_index.find(k);
      if(it == element_index.end()) {
        return element_list.end();
      }
      
      ++((it->second).second); // increase count
      return move_to_front((it->second).first);
    }
  
  ///////////////////////////////////////////////////////////
  // insert into the cache
  ///////////////////////////////////////////////////////////
  std::pair<iterator, bool> insert(const value_type& x)
    {
      assert(num_elem <= max_elem);
      
      // insert at front of list
      element_list.push_front(x);
      
      // insert into index
      typename IndexType::value_type index(x.first, IndexElementType(element_list.begin(), 1u));
      std::pair<typename IndexType::iterator, bool> index_insert_ret = element_index.insert(index);
      
      // If the value is found, do not insert anything.
      // Believe me this is compatible with SGI STL hash_map
      // 
      // Of course for us it also means that we need to move
      // value to the front of the list.
      if(!index_insert_ret.second) {
        element_list.pop_front();
        ++(((index_insert_ret.first)->second).second); // increase count
        return std::pair<iterator, bool>(move_to_front(((index_insert_ret.first)->second).first), false);
      }
      
      if(num_elem == max_elem) {
        // If we reached our max, remove last element
        // (by definition the LRU element).
        element_index.erase(element_list.back().first);  // first from index
        element_list.pop_back();                         // then from list
      }
      else {
        ++num_elem;
      }
      
      return std::pair<iterator, bool>(element_list.begin(), true);
    }
  
  ///////////////////////////////////////////////////////////
  // insert into the cache
  ///////////////////////////////////////////////////////////
  data_type& operator[](const key_type& k)
    {
      return (*((this->insert(value_type(k, data_type()))).first)).second;
    }
  
  ///////////////////////////////////////////////////////////
  // pass through and utility functions
  ///////////////////////////////////////////////////////////
  
  std::size_t size() 
    {
      assert(num_elem <= max_elem);
      return num_elem;
    }
  
  iterator       begin()       { return element_list.begin(); }
  const_iterator begin() const { return element_list.begin(); }
  
  iterator       end()         { return element_list.end(); }
  const_iterator end() const   { return element_list.end(); }
  
  void clear() 
    {
      num_elem = 0;
      element_list.clear();
      element_index.clear();
    }
  
  std::size_t count(KEY k) 
    {
      typename IndexType::iterator it = element_index.find(k);
      if(it == element_index.end()) {
        return 0u;
      }
      return (it->second).second;
    }
  
  std::size_t max_elements() { return max_elem; }
  std::size_t max_elements(std::size_t m) { return max_elem = m; }
    
  ///////////////////////////////////////////////////////////
  // friend functions
  ///////////////////////////////////////////////////////////
  
  friend bool operator==(const lru_cache& lhs, const lru_cache& rhs)
    {                                                                       
      // no need to check the index for equality
      return (lhs.max_elem == rhs.max_elem &&
              lhs.num_elem == rhs.num_elem &&
              lhs.element_list == rhs.element_list );
    }
  
  private:
  
  iterator move_to_front(iterator element_it)
    {
      if(element_it != element_list.begin()) {
        element_list.splice(element_list.begin(), element_list, element_it);
      }
      return element_it;
    }
  
  std::size_t max_elem;
  std::size_t num_elem;
  
  ListType  element_list;
  IndexType element_index;
};

#endif

#endif
