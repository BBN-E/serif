// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_STR_MATCH_NO_DT_NO_PRO_FIXED_DT
#define UT_FEATURE_STR_MATCH_NO_DT_NO_PRO_FIXED_DT

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
#include "Generic/UTCoref/featuretypes/UTFeatureSoonStrMatch.h"


/* 
 *
 * ported from features.py:str_match_no_dt_no_pro_fixed_dt
 */
class UTFeatureStrMatchNoDtNoProFixedDt : public UTFeatureType {

private:
   UTFeatureSoonStrMatch* ssm;

public:
   UTFeatureStrMatchNoDtNoProFixedDt() : UTFeatureType(Symbol(L"UTFeatureStrMatchNoDtNoProFixedDt")) {
	  ssm = static_cast<UTFeatureSoonStrMatch*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureSoonStrMatch")));
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  return ssm->detect(leftNode, rightNode, lam, true /*determiners*/, true /*pronouns*/, false /* premods */);
   }
};

#endif
