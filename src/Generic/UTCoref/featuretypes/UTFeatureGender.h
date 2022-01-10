// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_GENDER
#define UT_FEATURE_GENDER

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_GENDER
 */
class UTFeatureGender : public UTFeatureType {

public:
   UTFeatureGender() : UTFeatureType(Symbol(L"UTFeatureGender")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const
   {

	  Symbol lgender = lam.gender(leftNode);
	  Symbol rgender = lam.gender(rightNode);

	  if (lgender == LinkAllMentions::unknown || rgender == LinkAllMentions::unknown) {
		 return Unknown;
	  }

	  return lgender == rgender ? True : False;
   }
};

#endif
