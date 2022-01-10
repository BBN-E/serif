// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_LEFT_LOWER
#define UT_FEATURE_HEADWORD_LEFT_LOWER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* return the lowercased headword of leftNode
 *
 * ported from features.py:lower_headword_i
 */
class UTFeatureHeadwordLeftLower : public UTFeatureType {

public:
   UTFeatureHeadwordLeftLower() : UTFeatureType(Symbol(L"UTFeatureHeadwordLeftLower")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return leftNode.getHeadWord();
  }
};

#endif
