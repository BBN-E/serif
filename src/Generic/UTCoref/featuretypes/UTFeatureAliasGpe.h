// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_GPE
#define UT_FEATURE_ALIAS_GPE

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:gpe_alias
 */
class UTFeatureAliasGpe : public UTFeatureType {
public:

   UTFeatureAliasGpe() : UTFeatureType(Symbol(L"UTFeatureAliasGpe")) {

	  alias = static_cast<UTFeatureAliasSoon*>(
		 getFeatureType(getModel(), Symbol(L"UTFeatureAliasSoon")));

   }

private:
   UTFeatureAliasSoon *alias;

public:
  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
     return alias->detect(leftNode, rightNode, lam, true /*gpe is org */, false, false, false);
  }
};

#endif
