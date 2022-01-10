// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_LEFT_SUBSTR_RIGHT_NO_PRO
#define UT_FEATURE_LEFT_SUBSTR_RIGHT_NO_PRO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:j_substring_of_i_no_pro
 */
class UTFeatureLeftSubstrRightNoPro : public UTFeatureType {
private:
   UTFeatureLeftSubstrRight *lsr;

public:
   UTFeatureLeftSubstrRightNoPro() : UTFeatureType(Symbol(L"UTFeatureLeftSubstrRightNoPro")) {

      lsr = static_cast<UTFeatureLeftSubstrRight*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureLeftSubstrRight")));
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  if (lam.isPronoun(leftNode)) {
		 return False;
	  }
	  return lsr->x_substr_y(leftNode, rightNode, false /* ancestor ok */);
   }
};

#endif
