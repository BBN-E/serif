// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_SUPER
#define UT_FEATURE_ALIAS_SUPER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
#include "UTFeatureAliasSuperNoStrMatch.h"
#include "UTFeatureSoonStrMatch.h"
class SynNode;


/* 
 *
 * ported from features.py:super_alias
 */
class UTFeatureAliasSuper : public UTFeatureType {
public:

   UTFeatureAliasSuper() : UTFeatureType(Symbol(L"UTFeatureAliasSuper")) {

	  alias = static_cast<UTFeatureAliasSuperNoStrMatch*>(
		 getFeatureType(getModel(), Symbol(L"UTFeatureAliasSuperNoStrMatch")));

	  smatch = static_cast<UTFeatureSoonStrMatch*>(
		 getFeatureType(getModel(), Symbol(L"UTFeatureSoonStrMatch")));
   }

private:
   UTFeatureAliasSuperNoStrMatch *alias;
   UTFeatureSoonStrMatch *smatch;

public:
  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
	 
	 Symbol lname = lam.getName(leftNode);
	 Symbol rname = lam.getName(rightNode);

	 if (!lname.is_null() && lname == rname) {
		if (smatch->detect(leftNode, rightNode, lam, 
						   false /* allow pro */, 
						   false /* allow dt */, 
						   true /* no pomo */) == True) 
		{
		   return True;
		}
	 }

     return alias->detect(leftNode, rightNode, lam);
  }
};
#endif
