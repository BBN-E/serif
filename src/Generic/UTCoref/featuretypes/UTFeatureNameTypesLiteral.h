// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_NAME_TYPES_LITERAL
#define UT_FEATURE_NAME_TYPES_LITERAL

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"


/* what are the name types of these two nodes
 *
 * ported from features.py:types_literal
 */
class UTFeatureNameTypesLiteral : public UTFeatureType {

public:
   UTFeatureNameTypesLiteral() : UTFeatureType(Symbol(L"UTFeatureNameTypesLiteral")) {}

   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {

	  Symbol lname = lam.getName(leftNode);
	  Symbol rname = lam.getName(rightNode);

	  if (lname.is_null()) {
		 lname = None;
	  }
	  if (rname.is_null()) {
		 rname = None;
	  }


	  std::wstring s;
	  s += rname.to_string();
	  s += L",";
	  s += lname.to_string();

	  return Symbol(s);
  }
};

#endif
