// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_RIGHT
#define UT_FEATURE_HEADWORD_RIGHT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* return the headword of rightNode
 *
 * ported from features.py:headword_j
 */
class UTFeatureHeadwordRight : public UTFeatureType {

public:
   UTFeatureHeadwordRight() : UTFeatureType(Symbol(L"UTFeatureHeadwordRight")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return Symbol(lam.casedString(*(rightNode.getHeadPreterm())));
  }
};

#endif
