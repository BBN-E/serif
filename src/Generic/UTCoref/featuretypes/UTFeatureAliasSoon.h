// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_SOON
#define UT_FEATURE_ALIAS_SOON

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_ALIAS
 */
class UTFeatureAliasSoon : public UTFeatureType {
public:
  UTFeatureAliasSoon() : UTFeatureType(Symbol(L"UTFeatureAliasSoon")) {

	 alias_date = static_cast<UTFeatureAliasDate*>(
		getFeatureType(getModel(), Symbol(L"UTFeatureAliasDate")));

	 alias_person = static_cast<UTFeatureAliasPerson*>(
		getFeatureType(getModel(), Symbol(L"UTFeatureAliasPerson")));

	 alias_org = static_cast<UTFeatureAliasOrg*>(
		getFeatureType(getModel(), Symbol(L"UTFeatureAliasOrg")));
  }

private:
  UTFeatureAliasDate *alias_date;
  UTFeatureAliasPerson *alias_person;
  UTFeatureAliasOrg *alias_org;

public:

  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
     return detect(leftNode, rightNode, lam, false, false, false, false);
  }

  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam,
                bool gpe_is_org=false,
                bool ignore_lower=false,
                bool of_alias=false,
                bool org_match_first_word=false) const
  {
	 if (alias_date->detect(leftNode, rightNode, lam) == True) {
		return True;
	 }

	 if (alias_person->detect(leftNode, rightNode, lam) == True) {
		return True;
	 }

	 return alias_org->detect(leftNode, rightNode, lam, gpe_is_org, ignore_lower, of_alias, org_match_first_word);
  }
};

#endif
