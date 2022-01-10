// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_A_OF_B_IS_B_A
#define UT_FEATURE_A_OF_B_IS_B_A

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"


/*
 *
 * ported from features.py:a_of_b_is_ba
 */
class UTFeatureAOfBIsBA : public UTFeatureType {

public:

   UTFeatureAOfBIsBA() : UTFeatureType(Symbol(L"UTFeatureAOfBIsBA")) {}

   bool check(const std::wstring &astring,
			  const std::wstring &bstring) const {
	  
	  size_t ofloc = astring.find(L" of ");
	  
	  if (ofloc == std::wstring::npos) {
		 return false;
	  }
	  
	  const std::wstring swapped = (astring.substr(0, ofloc) +
									astring.substr(ofloc + 4 /*len(" of ")*/));
	  return swapped == bstring;
   }
   
   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  
	  const std::wstring lstring = lam.casedString(leftNode);
	  const std::wstring rstring = lam.casedString(rightNode);
	  
	  return (check(lstring, rstring) || check(rstring, lstring)) ? True : False;
   }
};

#endif
