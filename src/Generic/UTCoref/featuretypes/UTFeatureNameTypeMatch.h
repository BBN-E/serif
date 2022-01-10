// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_NAME_TYPE_MATCH_H
#define UT_FEATURE_NAME_TYPE_MATCH_H

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"

class SynNode;


/* do these two nodes have the same name type (including both unnamed)?
 *
 * ported from features.py:types_consistent
 */
class UTFeatureNameTypeMatch : public UTFeatureType {

public:
   UTFeatureNameTypeMatch() : UTFeatureType(Symbol(L"UTFeatureNameTypeMatch")) {}

   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {
	  Symbol lname = lam.getName(leftNode);
	  Symbol rname = lam.getName(rightNode);
	  return lname == rname ? True : False;
   }
};

#endif
