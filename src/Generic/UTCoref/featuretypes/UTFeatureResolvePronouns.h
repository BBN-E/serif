// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_RESOLVE_PRONOUNS
#define UT_FEATURE_RESOLVE_PRONOUNS

#include <string>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:resolve_pronouns
 */
class UTFeatureResolvePronouns : public UTFeatureType {
private:
   UTFeatureType *gender;
   UTFeatureType *number;
   UTFeatureType *diffsents;

public:
   UTFeatureResolvePronouns() : UTFeatureType(Symbol(L"UTFeatureResolvePronouns")) {

      gender = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureGender")));

      number = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureNumber")));

      diffsents = static_cast<UTFeatureType*>(
         getFeatureType(getModel(), Symbol(L"UTFeatureDifferentSentences")));
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {

	  //std::cout << "{" << leftNode.toTextString() << "," << rightNode.toTextString() << "}" << std::endl;
	  //std::cout << "  " << lam.isPronoun(rightNode) << " " << leftNode.isAncestorOf(&rightNode) << (lam.gender(leftNode) != lam.gender(rightNode))
	  //			<< (lam.number(leftNode) != lam.number(rightNode)) << lam.isPronoun(leftNode) << (lam.lookupParseNumber(leftNode, *lam.parses))
	  //		<< lam.lookupParseNumber(rightNode, *lam.parses) << std::endl;

	  if (!lam.isPronoun(rightNode) ||                      /* only apply to pronouns */
		  leftNode.isAncestorOf(&rightNode) ||              /* pronouns are not coreferent with ancestor nodes */
		  gender->detect(leftNode,rightNode,lam) == False ||  /* gender must match */
		  number->detect(leftNode,rightNode,lam) == False)    /* number must match */
	  {
		 return False;
	  }

	  if (lam.isPronoun(leftNode)) {
		 return True;
	  }

	  return diffsents->detect(leftNode,rightNode,lam) == False ? True : False;
	  
   }
};

#endif
