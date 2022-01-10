// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_WORDNET_LCA
#define UT_FEATURE_WORDNET_LCA

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:wordnet_lca
 */
class UTFeatureWordnetLca : public UTFeatureType {

public:
   UTFeatureWordnetLca() : UTFeatureType(Symbol(L"UTFeatureWordnetLca")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	 std::wstring lca;

	 Symbol lname = leftNode.getHeadWord();
     Symbol rname = rightNode.getHeadWord();
	 
	 std::wstring word1, word2;
	 word1 = lname.to_string();
	 word2 = rname.to_string();

	 lam.utwl.lca(word1, word2, lca);

	 return Symbol(lca);
  }
};

#endif
