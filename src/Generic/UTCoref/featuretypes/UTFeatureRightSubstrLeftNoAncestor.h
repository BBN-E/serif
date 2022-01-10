// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_RIGHT_SUBSTR_NO_ANCESTOR
#define UT_FEATURE_RIGHT_SUBSTR_NO_ANCESTOR

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:j_substring_of_i_no_ancestor
 */
class UTFeatureRightSubstrLeftNoAncestor : public UTFeatureType {
private:
   UTFeatureLeftSubstrRight *lsr;

public:
   UTFeatureRightSubstrLeftNoAncestor() : UTFeatureType(Symbol(L"UTFeatureRightSubstrLeftNoAncestor")) {

      lsr = static_cast<UTFeatureLeftSubstrRight*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureLeftSubstrRight")));
   }

   Symbol detect(const SynNode &leftNode,
			   const SynNode &rightNode,
			   const LinkAllMentions &lam) const {
	  
	  return lsr->x_substr_y(rightNode, leftNode, true /* no ancestor */);
   }
};

#endif
