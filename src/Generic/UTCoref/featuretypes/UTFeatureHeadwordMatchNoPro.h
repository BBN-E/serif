// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_MATCH_NO_PRO
#define UT_FEATURE_HEADWORD_MATCH_NO_PRO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
#include "Generic/UTCoref/featuretypes/UTFeatureHeadwordMatch.h"
class SynNode;


/* do the nodes have the same head word considering case and counting
 * pronouns as not matching?
 *
 * ported from features.py:headword_match_no_pro
 */
class UTFeatureHeadwordMatchNoPro : public UTFeatureType {
private:
   UTFeatureHeadwordMatch *hwm;

public:
   
   UTFeatureHeadwordMatchNoPro() : UTFeatureType(Symbol(L"UTFeatureHeadwordMatchNoPro")) {

	  hwm = static_cast<UTFeatureHeadwordMatch*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureHeadwordMatch")));
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  if (lam.isPronoun(rightNode)) {
		 return False;
	  }
	  else {
		 return hwm->detect(leftNode, rightNode, lam);
	  }
   }
};

#endif
