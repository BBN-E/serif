// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_PERSON
#define UT_FEATURE_ALIAS_PERSON

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:alias_person
 */
class UTFeatureAliasPerson : public UTFeatureType {

public:
   UTFeatureAliasPerson() : UTFeatureType(Symbol(L"UTFeatureAliasPerson")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
     
	  if (lam.getName(leftNode) != lam.per || lam.getName(rightNode) != lam.per) {
		 return False;
	  }
	  return (leftNode.getLastTerminal()->getTag() == rightNode.getLastTerminal()->getTag()) ? True : False;
   }
};

#endif
