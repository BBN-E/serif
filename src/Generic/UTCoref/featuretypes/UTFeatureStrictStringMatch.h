// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_STRICT_STRING_MATCH_H
#define UT_FEATURE_STRICT_STRING_MATCH_H

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"

/* are the nodes exactly the same?
 *
 * ported from features.py:strict_string_match
 */
class UTFeatureStrictStringMatch : public UTFeatureType {

public:
   UTFeatureStrictStringMatch() : UTFeatureType(L"UTFeatureStrictStringMatch") {}

   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {
   
	  const std::wstring lstring = lam.casedString(leftNode);
	  const std::wstring rstring = lam.casedString(rightNode);

	  return lstring == rstring ? True : False;
  }
};

#endif
