// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_MATCH_LOWER
#define UT_FEATURE_HEADWORD_MATCH_LOWER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"


/* do the nodes have the same head word?
 *
 * ported from features.py:headword_match_lower
 */
class UTFeatureHeadwordMatchLower : public UTFeatureType {

public:
   UTFeatureHeadwordMatchLower() : UTFeatureType(Symbol(L"UTFeatureHeadwordMatchLower")) {}

   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {

	  Symbol lname = leftNode.getHeadWord();
	  Symbol rname = rightNode.getHeadWord();

	  return leftNode.getHeadWord() == rightNode.getHeadWord() ? True : False;
   }
};

#endif
