// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_SOME_SUBSTRING_H
#define UT_FEATURE_SOME_SUBSTRING_H

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* is one node a substring of the other?
 *
 * ported from features.py:names_consistent
 */
class UTFeatureSomeSubstring : public UTFeatureType {

public:
   UTFeatureSomeSubstring() : UTFeatureType(Symbol(L"UTFeatureSomeSubstring")) {}
	  
   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	 const std::wstring lstring = lam.casedString(leftNode);
	 const std::wstring rstring = lam.casedString(rightNode);
	 
	 return (lstring.find(rstring) != std::wstring::npos ||
			 rstring.find(lstring) != std::wstring::npos) ? True : False;
  }
};

#endif
