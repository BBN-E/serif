// Copyright 2013 Raytheon BBN Technologies 
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Urdu/trainers/ur_HeadFinder.h"
#include "Generic/common/Symbol.h"
#include "Generic/trainers/Production.h"
#include <boost/algorithm/string.hpp>



UrduHeadFinder::UrduHeadFinder() {
	
}

int UrduHeadFinder::get_head_index()
{
  for (int i=0; i < production.number_of_right_side_elements; ++i) 
    if (boost::starts_with(production.right[i].to_string(),Symbol(L"HEAD").to_string()))
      return i;
        
  return -1;
}


UrduHeadFinder::~UrduHeadFinder() {
}


