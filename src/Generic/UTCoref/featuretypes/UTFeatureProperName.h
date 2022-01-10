// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_PROPER_NAME
#define UT_FEATURE_PROPER_NAME

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_PROPER_NAME
 */
class UTFeatureProperName : public UTFeatureType {

public:
   UTFeatureProperName() : UTFeatureType(Symbol(L"UTFeatureProperName")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  return (lam.isProper(leftNode) && lam.isProper(rightNode)) ? True : False;
   }
};

#endif
