// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_SOON_STR_MATCH
#define UT_FEATURE_SOON_STR_MATCH

#include <string>
#include "boost/foreach.hpp"
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/WordConstants.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/*
 *
 * ported from features.py:soon_STR_MATCH
 */
class UTFeatureSoonStrMatch : public UTFeatureType {

public:
   UTFeatureSoonStrMatch() : UTFeatureType(Symbol(L"UTFeatureSoonStrMatch")) {}

   void toString(bool exclude_determiners, bool exclude_premodifiers, const SynNode &node,
				 const LinkAllMentions &lam, std::wstring &s) const {
	  std::vector<const SynNode*> terminals;
	  node.getAllTerminalNodes(terminals);
	  BOOST_FOREACH(const SynNode* terminal, terminals) {
		 if ( (!exclude_determiners || !lam.isDeterminer(*terminal)) &&
			  (!exclude_premodifiers || !lam.is_premodifier(terminal->getTag())))
		 {
			s += terminal->getTag().to_string(); // with trailing space
		 }
	  }
   }

   Symbol detect(const SynNode &leftNode,
                 const SynNode &rightNode,
                 const LinkAllMentions &lam) const {
	  return detect(leftNode, rightNode, lam, true /* dt */, false /* pro */, false /*premod*/);
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam,
				 bool exclude_determiners,
				 bool exclude_pronouns,
				 bool exclude_premodifiers) const {

	  if (exclude_pronouns && (lam.isPronoun(leftNode) ||
							   lam.isPronoun(rightNode))) {
		 return False;
	  }

	  std::wstring ls, rs;

	  toString(exclude_determiners, exclude_premodifiers, leftNode, lam, ls);
	  toString(exclude_determiners, exclude_premodifiers, rightNode, lam, rs);

	  /*
	  std::cout << "***:" << std::endl;

	  std::cout << "  " << getName().to_debug_string() << ": \"" 
				<< std::wstring(leftNode.toTextString()) << "\" \"" << ls << '"' << std::endl;

	  std::cout << "  " << getName().to_debug_string() << ": \"" 
				<< std::wstring(rightNode.toTextString()) << "\" \"" << rs << '"' << std::endl;

	  std::cout << (ls == rs ? True : False).to_debug_string() << std::endl;
	  */

	  return ls == rs ? True : False;
  }
};

#endif
