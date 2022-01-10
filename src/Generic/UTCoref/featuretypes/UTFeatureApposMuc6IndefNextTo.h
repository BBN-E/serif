// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_APPOS_MUC6_INDEF_NEXTTO
#define UT_FEATURE_APPOS_MUC6_INDEF_NEXTTO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_APPOS_nextto_MUC_6_indef
 */
class UTFeatureApposMuc6IndefNextTo : public UTFeatureType {
public:
   UTFeatureApposMuc6IndefNextTo() : UTFeatureType(Symbol(L"UTFeatureApposMuc6IndefNextTo")) {

      appos = static_cast<UTFeatureApposMuc6NextTo*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureApposMuc6NextTo")));
   }

private:
   UTFeatureApposMuc6NextTo* appos;

public:
   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return appos->detect(leftNode, rightNode, lam, true /* muc_version_6 */, true /*indef */);
   }
};

#endif
