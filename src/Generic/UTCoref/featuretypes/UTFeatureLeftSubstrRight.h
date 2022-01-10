// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_LEFT_SUBSTR_RIGHT
#define UT_FEATURE_LEFT_SUBSTR_RIGHT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:j_substring_of_i
 */
class UTFeatureLeftSubstrRight : public UTFeatureType {

public:

   UTFeatureLeftSubstrRight() : UTFeatureType(Symbol(L"UTFeatureLeftSubstrRight")) {}

   Symbol x_substr_y(const SynNode &x,
					 const SynNode &y,
					 bool no_ancestor) const {
	  if (no_ancestor && y.isAncestorOf(&x)) {
		 return False;
	  }

	  std::wstring sx = x.toTextString();
	  std::wstring sy = y.toTextString();

      
	  sx.resize(sx.length()-1); // delete trailing space

	  return sy.find(sx) != std::wstring::npos ? True : False;	  	  
   }

  Symbol detect(const SynNode &leftNode,
              const SynNode &rightNode,
              const LinkAllMentions &lam) const 
  {
	 return x_substr_y(leftNode, rightNode, false /* ancestor ok */);
  }
};

#endif
