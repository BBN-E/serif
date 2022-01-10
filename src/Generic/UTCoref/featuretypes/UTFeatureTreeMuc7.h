// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_TREE_MUC7
#define UT_FEATURE_TREE_MUC7

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_TREE_MUC_7
 */
class UTFeatureTreeMuc7 : public UTFeatureType {
private:
   UTFeatureTreeMuc6 *tree;

public:
   UTFeatureTreeMuc7() : UTFeatureType(Symbol(L"UTFeatureTreeMuc7")) {

      tree = static_cast<UTFeatureTreeMuc6*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureTreeMuc6")));

   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return tree->detect(leftNode, rightNode, lam, false /* not muc6 */);
   }
};

#endif
