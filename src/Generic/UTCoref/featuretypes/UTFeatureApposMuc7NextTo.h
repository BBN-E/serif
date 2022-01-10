// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_APPOS_MUC7_NEXTTO
#define UT_FEATURE_APPOS_MUC7_NEXTTO

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:soon_APPOS_nextto_MUC_7
 */
class UTFeatureApposMuc7NextTo : public UTFeatureType {
public:
   UTFeatureApposMuc7NextTo() : UTFeatureType(Symbol(L"UTFeatureApposMuc7NextTo")) {

      appos = static_cast<UTFeatureApposMuc6NextTo*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureApposMuc6NextTo")));

   }

private:
   UTFeatureApposMuc6NextTo* appos;

public:
   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return appos->detect(leftNode, rightNode, lam, false /* muc_version_6 */, false /*indef */);
   }
};

#endif
