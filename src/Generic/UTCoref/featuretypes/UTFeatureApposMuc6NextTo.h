// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_APPOS_MUC6_NEXTTO
#define UT_FEATURE_APPOS_MUC6_NEXTTO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_APPOS_nextto_MUC_6
 */
class UTFeatureApposMuc6NextTo : public UTFeatureType {

public:
   UTFeatureApposMuc6NextTo() : UTFeatureType(Symbol(L"UTFeatureApposMuc6NextTo")) {}

   Symbol detect(const SynNode &leftNode,
                 const SynNode &rightNode,
                 const LinkAllMentions &lam) const {
	  return detect(leftNode, rightNode, lam, true /* muc_version_6 */, false /*indef */);
   }

   Symbol detect(const SynNode &leftNode,
                 const SynNode &rightNode,
                 const LinkAllMentions &lam,
				 bool muc_version_6,
				 bool indef) const {

	  static Symbol comma(L",");

	  const SynNode *i = &leftNode;
	  const SynNode *j = &rightNode;

	  if (i == j) {
		 return False;
	  }

	  if (muc_version_6 && !indef && !lam.isDefinite(*j)) {
		 return False;
	  }

	  if (muc_version_6 && indef && lam.isIndefinite(*j)) {
		 return False;
	  }

	  if (!lam.isProper(*i) && !lam.isProper(*j)) {
		 return False;
	  }

	  if (i->getParent() != j->getParent()) {
		 return False;
	  }

	  if (j->getTag() == Symbol(L"NPP") && j->getNChildren() == 1 &&
		  j->getChild(0)->isPreterminal() && lam.getName(*j) != lam.per)
	  {
		 return False;
	  }

	  if (i != i->getParent()->getChild(0)) {
		 return False;
	  }

	  if (j == i->getParent()->getChild(0)) {
         return False; // need intevening punct
      }

	  for (int c = 1 ; c < i->getParent()->getNChildren() ; c++) {
		 const SynNode* k = i->getParent()->getChild(c);
		 if (k == j) {
			continue;
		 }
		 if (!k->isPreterminal() || k->getSingleWord() != comma) {
			return False;
		 }
	  }
	  return True;
   }
};

#endif
