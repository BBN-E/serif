// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_TREE_MUC6
#define UT_FEATURE_TREE_MUC6

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_TREE_MUC_6
 */
class UTFeatureTreeMuc6 : public UTFeatureType {
private:
   UTFeatureType *appos6;
   UTFeatureType *appos7;
   UTFeatureType *strmatch;
   UTFeatureType *resolvepros;
   UTFeatureType *alias;

public:
   UTFeatureTreeMuc6() : UTFeatureType(Symbol(L"UTFeatureTreeMuc6")) {

      appos6 = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureApposMuc6NextTo")));

      appos7 = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureApposMuc7NextTo")));

      strmatch = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureSoonStrMatch")));

      resolvepros = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureResolvePronouns")));	  

      alias = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureAliasSoon")));	  

   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam,
				 bool muc6) const {

	  if (strmatch->detect(leftNode,rightNode,lam) == True) {
		 return True;
	  }

	  if (!lam.isPronoun(rightNode)) {
		 if ((muc6 ? appos6 : appos7)->detect(leftNode,rightNode,lam) == True) {
			return True;
		 }
		 return alias->detect(leftNode,rightNode,lam);
	  }

	  return resolvepros->detect(leftNode,rightNode,lam);
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return detect(leftNode, rightNode, lam, true /* muc6 */);
   }
};

#endif
