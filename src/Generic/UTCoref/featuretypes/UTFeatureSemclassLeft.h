// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_SEMCLASS_LEFT
#define UT_FEATURE_SEMCLASS_LEFT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:semclass_I
 */
class UTFeatureSemclassLeft : public UTFeatureType {

public:
   UTFeatureSemclassLeft() : UTFeatureType(Symbol(L"UTFeatureSemclassLeft")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  
	  return lam.semclass(leftNode);
   }
};

#endif
