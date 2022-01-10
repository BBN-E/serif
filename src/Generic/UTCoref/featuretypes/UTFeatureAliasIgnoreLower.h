// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_IGNORE_LOWER
#define UT_FEATURE_ALIAS_IGNORE_LOWER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:ignore_lower_alias
 */
class UTFeatureAliasIgnoreLower : public UTFeatureType {
public:

   UTFeatureAliasIgnoreLower() : UTFeatureType(Symbol(L"UTFeatureAliasIgnoreLower")) {

	  alias = static_cast<UTFeatureAliasSoon*>(
		 getFeatureType(getModel(), Symbol(L"UTFeatureAliasSoon")));

   }

private:
   UTFeatureAliasSoon *alias;

public:
  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
     return alias->detect(leftNode, rightNode, lam, false, true /* ignore lower */, false, false);
  }
};

#endif
