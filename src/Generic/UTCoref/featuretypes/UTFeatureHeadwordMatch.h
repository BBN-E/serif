// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_HEADWORD_MATCH
#define UT_FEATURE_HEADWORD_MATCH

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;

/* do the nodes have the same head word considering case?
 *
 * ported from features.py:headword_match
 */
class UTFeatureHeadwordMatch : public UTFeatureType {

public:
   UTFeatureHeadwordMatch() : UTFeatureType(Symbol(L"UTFeatureHeadwordMatch")) {}

   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {
	  const SynNode *l_head_preterm = leftNode.getHeadPreterm();
	  const SynNode *r_head_preterm = rightNode.getHeadPreterm();
	 
	  return lam.casedString(*l_head_preterm) == lam.casedString(*r_head_preterm) ? True : False;
   }
};

#endif
