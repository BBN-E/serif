// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_LEFT
#define UT_FEATURE_HEADWORD_LEFT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* return the headword of leftNode
 *
 * ported from features.py:headword_i
 */
class UTFeatureHeadwordLeft : public UTFeatureType {

public:
  UTFeatureHeadwordLeft() : UTFeatureType(Symbol(L"UTFeatureHeadwordLeft")) {}

  Symbol detect(const SynNode &leftNode,
				const SynNode &rightNode,
				const LinkAllMentions &lam) const {

	 return Symbol(lam.casedString(*(leftNode.getHeadPreterm())));
  }
};

#endif
