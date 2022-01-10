// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_RIGHT_LOWER
#define UT_FEATURE_HEADWORD_RIGHT_LOWER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* return the lowercased headword of rightNode
 *
 * ported from features.py:lower_headword_i
 */
class UTFeatureHeadwordRightLower : public UTFeatureType {

public:
   UTFeatureHeadwordRightLower() : UTFeatureType(Symbol(L"UTFeatureHeadwordRightLower")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return rightNode.getHeadWord();
   }
};

#endif
