// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_SEMCLASS_RIGHT
#define UT_FEATURE_SEMCLASS_RIGHT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:semclass_J
 */
class UTFeatureSemclassRight : public UTFeatureType {

public:
   UTFeatureSemclassRight() : UTFeatureType(Symbol(L"UTFeatureSemclassRight")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return lam.semclass(rightNode);
   }
};

#endif
