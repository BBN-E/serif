// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_RIGHT_IS_PRO
#define UT_FEATURE_RIGHT_IS_PRO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* is rightNode a pronoun?
 *
 * ported from features.py:soon_J_PRONOUN
 */
class UTFeatureRightIsPro : public UTFeatureType {

public:
   UTFeatureRightIsPro() : UTFeatureType(Symbol(L"UTFeatureRightIsPro")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return lam.isPronoun(rightNode) ? True : False;
   }
};

#endif
