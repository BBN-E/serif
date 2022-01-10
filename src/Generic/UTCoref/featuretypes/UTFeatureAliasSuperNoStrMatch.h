// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_SUPER_NO_STR_MATCH
#define UT_FEATURE_ALIAS_SUPER_NO_STR_MATCH

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
#include "UTFeatureAliasSoon.h"
class SynNode;


/* 
 *
 * ported from features.py:super_alias_nostrmatch
 */
class UTFeatureAliasSuperNoStrMatch : public UTFeatureType {
public:

   UTFeatureAliasSuperNoStrMatch() : UTFeatureType(Symbol(L"UTFeatureAliasSuperNoStrMatch")) {

	  alias = static_cast<UTFeatureAliasSoon*>(
		 getFeatureType(getModel(), Symbol(L"UTFeatureAliasSoon")));
   }

private:
   UTFeatureAliasSoon *alias;

public:
  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
     return alias->detect(leftNode, rightNode, lam, true /* org is gpe */, true /* ignore lower */, true /* of */, true /* org first word */);
  }
};

#endif
