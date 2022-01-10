/******************************************************************
* Copyright 2008 by BBN Technologies Corp.     All Rights Reserved
******************************************************************/

#include "License/nemesis/utilizing/string_utils.hpp"


namespace nemesis 
{

  std::vector<char> string_to_vector(const std::string& str)
  {
    std::vector<char> tmp_vec(str.size() + 1, '\0');
    std::copy(str.begin(), str.end(), tmp_vec.begin());
    return tmp_vec;
  }

  template<>
  void split_string(std::string::const_iterator str_begin,
                    std::string::const_iterator str_end,
                    std::back_insert_iterator<std::vector<std::vector<char> > > out_begin,
                    const boost::regex& re)
  {
    // first split the string
    std::vector<std::string> tmp;
    split_string(str_begin, str_end, std::back_inserter(tmp), re);

    // then copy the (null terminated) bytes
    std::transform(tmp.begin(), tmp.end(), out_begin, string_to_vector);
  }

}
