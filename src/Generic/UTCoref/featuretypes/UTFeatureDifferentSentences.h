// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_DIFFERENT_SENTENCES
#define UT_FEATURE_DIFFERENT_SENTENCES

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:soon_DIST_gt_0
 */
class UTFeatureDifferentSentences : public UTFeatureType {

public:
   UTFeatureDifferentSentences() : UTFeatureType(Symbol(L"UTFeatureDifferentSentences")) {}
   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  return (lam.lookupParseNumber(leftNode, *lam.parses) != 
			  lam.lookupParseNumber(rightNode, *lam.parses)) ? True : False;
   }
};

#endif
