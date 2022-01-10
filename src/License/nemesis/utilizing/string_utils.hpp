/******************************************************************
* Copyright 2008 by BBN Technologies Corp.     All Rights Reserved
******************************************************************/

#ifndef CUBE2_NEMESIS_UTILIZING_STRING_UTILS_HPP
#define CUBE2_NEMESIS_UTILIZING_STRING_UTILS_HPP

/*
// #define CUBE2_CHAR_STAR_CAST(vector_ref)   \
//               &((vector_ref)[0]);     \
//               BOOST_STATIC_ASSERT((boost::is_same<typeof(vector_ref), std::vector<char> >::value))
*/

#include <boost/regex.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <vector>
#include <string>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cctype> // std::toupper, std::tolower
#include <iostream>

namespace nemesis
{
  //////////////////////////////////////////////////////////////
  // the third parameter of string_to() should be 
  // one of std::hex, std::dec or std::octtemplate <class T>
  //////////////////////////////////////////////////////////////
  template<class T>
  T string_to(const std::string& s, 
              std::ios_base& (*f)(std::ios_base&) = std::dec)
  {
    T t;
    std::istringstream iss(s);

    if((iss >> f >> t).fail()) {
      throw std::invalid_argument("string_to<T>(): conversion failed");
    }

    return t;
  }

  //////////////////////////////////////////////////////////////
  // the third parameter of to_string() should be 
  // one of std::hex, std::dec or std::octtemplate <class T>
  //////////////////////////////////////////////////////////////
  template<class T>
  std::string to_string(T val,
                        std::ios_base& (*f)(std::ios_base&) = std::dec)
  {
    std::ostringstream oss;

    if((oss << f << val).fail()) {
      throw std::invalid_argument("to_string<T>(): conversion failed");
    }
    
    return oss.str();
  }

  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  std::vector<char> string_to_vector(const std::string& str);
  
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////
  template<class T>
  typename boost::remove_pointer<T>::type* 
  vector_star_cast(std::vector< typename boost::remove_pointer<T>::type >& vector_ref)
  {
    return &(vector_ref[0]);
  }

  //////////////////////////////////////////////////////////////
  // split a string STL style
  //////////////////////////////////////////////////////////////
  template<class ITERATOR>
  void split_string(std::string::const_iterator str_begin,
                    std::string::const_iterator str_end,
                    ITERATOR out_begin,
                    const boost::regex& re)
  {
    boost::sregex_token_iterator it_begin(str_begin, str_end, re, -1);
    boost::sregex_token_iterator it_end;
    std::copy(it_begin, it_end, out_begin);
  }
  
  //////////////////////////////////////////////////////////////
  // split a string STL style
  //////////////////////////////////////////////////////////////
  template<>
  void split_string(std::string::const_iterator str_begin,
                    std::string::const_iterator str_end,
                    std::back_insert_iterator<std::vector<std::vector<char> > > out_begin,
                    const boost::regex& re);
  
  //////////////////////////////////////////////////////////////
  // convenience function, split a string Perl style
  //////////////////////////////////////////////////////////////
  inline std::vector<std::string> split_string(std::string str,
                                               std::string re_str = std::string(" "))
  {
    // Because we have not passed the strings by reference we are
    // operating in our own (compiler given) copies.
    // http://cpp-next.com/archive/2009/08/want-speed-pass-by-value/

    if(re_str == " ") {
      // trim to emulate Perl's split(' ') semantics.
      boost::trim(str);
      re_str = "\\s+";
    }

    std::vector<std::string> retvec;
    split_string(str.begin(), str.end(), std::back_inserter(retvec), boost::regex(re_str));
    return retvec; // by value
  }

  //////////////////////////////////////////////////////////////
  // convenience function, split a c-string Perl style
  //////////////////////////////////////////////////////////////
  inline std::vector<std::string> split_string(const char* c_str,
                                               const std::string re_str = std::string(" "))
  {
    return split_string(std::string(c_str), re_str);
  }

  //////////////////////////////////////////////////////////////
  // convenience function, split a string Perl style
  //////////////////////////////////////////////////////////////
  inline std::vector<std::vector<char> >split_string_vec(std::string str,
                                                         std::string re_str = std::string(" "))
  {
    // Because we have not passed the strings by reference we are
    // operating in our own (compiler given) copies.
    // http://cpp-next.com/archive/2009/08/want-speed-pass-by-value/

    if(re_str == " ") {
      // trim to emulate Perl's split(' ') semantics.
      boost::trim(str);
      re_str = "\\s+";
    }

    std::vector<std::vector<char> > retvec;
    split_string(str.begin(), str.end(), std::back_inserter(retvec), boost::regex(re_str));
    return retvec; // by value
  }
  
  //////////////////////////////////////////////////////////////
  // convenience function, split and cast
  //////////////////////////////////////////////////////////////
  template<class T>
  inline std::vector<T> split(const std::string& str,
                              const std::string& reg_str = "\\s+"
                              )
  {
    std::vector<T> retvec;
    std::vector<std::string> tmp = split_string(str, reg_str);
    for ( unsigned i = 0; i < tmp.size(); ++i ) 
      retvec.push_back( boost::lexical_cast<T>(tmp[i]) );
    return retvec;
  }

  //////////////////////////////////////////////////////////////
  // join strings together Perl style
  //////////////////////////////////////////////////////////////
  template<class ITERATOR>
  inline std::string join_strings( ITERATOR begin, 
                                   ITERATOR end,
                                   const std::string& delim = " " )
  {
    ITERATOR it = begin;
    std::string res("");
    for ( ; it != end; ++it ) {
      if ( it != begin )
        res += delim;
      res += *it;
    }
    return res;
  }

  inline std::string join_strings(const std::vector<std::string>& str, const std::string& delim = " ")
  {
    return join_strings(str.begin(),str.end(),delim);
  }

  //////////////////////////////////////////////////////////////
  // join values together using lexical_cast
  //////////////////////////////////////////////////////////////
  template<class ITERATOR>
  inline std::string join ( ITERATOR begin,
                            ITERATOR end,
                            const std::string& delim = " " )
  {
    ITERATOR it = begin;
    std::string res("");
    for ( ; it != end; ++it ) {
      if ( it != begin )
        res += delim;
      res += boost::lexical_cast<std::string>(*it);
    }
    return res;
  }


  //////////////////////////////////////////////////////////////
  // convert to uppercase 
  //////////////////////////////////////////////////////////////
  template<class charT, class traits, class Alloc>
  std::basic_string<charT, traits, Alloc>& to_upper(std::basic_string<charT, traits, Alloc>& str)
  {
    std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
    return str;
  }

  //////////////////////////////////////////////////////////////
  // convert to uppercase
  // notice this one returns a new string by value
  // also use default traits and allocator :(
  //////////////////////////////////////////////////////////////
  template<class charT>
  std::basic_string<charT> to_upper(const charT* str)
  {
    std::basic_string<charT> retstr(str);
    return to_upper(retstr);
  }


  //////////////////////////////////////////////////////////////
  // convert to lowercase
  //////////////////////////////////////////////////////////////
  template<class charT, class traits, class Alloc>
  std::basic_string<charT, traits, Alloc>& to_lower(std::basic_string<charT, traits, Alloc>& str)
  {
    std::transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
    return str;
  }

  //////////////////////////////////////////////////////////////
  // convert to lowercase
  // notice this one returns a new string by value
  // also use default traits and allocator :(
  //////////////////////////////////////////////////////////////
  template<class charT>
  std::basic_string<charT> to_lower(const charT* str)
  {
    std::basic_string<charT> retstr(str);
    return to_lower(retstr);
  }
  
}



#endif
