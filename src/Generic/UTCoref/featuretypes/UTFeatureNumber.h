// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_NUMBER
#define UT_FEATURE_NUMBER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_NUMBER
 */
class UTFeatureNumber : public UTFeatureType {

public:
  UTFeatureNumber() : UTFeatureType(Symbol(L"UTFeatureNumber")) {}

  Symbol detect(const SynNode &leftNode,
				const SynNode &rightNode,
				const LinkAllMentions &lam) const {

	 return lam.number(leftNode) == lam.number(rightNode) ? True : False;
  }
};

#endif
