// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_SEMCLASS
#define UT_FEATURE_SEMCLASS

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_SEMCLASS
 */
class UTFeatureSemclass : public UTFeatureType {

private:

   bool detect(Symbol a, Symbol b) const {
	  if (a == LinkAllMentions::object && (b == LinkAllMentions::organization ||
												  b == LinkAllMentions::location ||
												  b == LinkAllMentions::date ||
												  b == LinkAllMentions::time ||
												  b == LinkAllMentions::money ||
												  b == LinkAllMentions::percent))
	  {
		 return true;
	  }

	  if (a == LinkAllMentions::person && (b == LinkAllMentions::male ||
												  b == LinkAllMentions::female))
	  {
		 return true;
	  }

	  return false;
   }

public:

   UTFeatureSemclass() : UTFeatureType(Symbol(L"UTFeatureSemclass")) {}

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  Symbol sleft = lam.semclass(leftNode);
	  Symbol sright = lam.semclass(rightNode);

	  if (sleft == LinkAllMentions::unknown || sright == LinkAllMentions::unknown) {
		 return Unknown;
	  }
	  else {
		 return (sleft == sright || detect(sleft,sright) || detect(sright,sleft)) ? True : False;
	  }
   }
};

#endif
