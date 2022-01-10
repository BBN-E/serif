// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_EXACT_MODIFIER_HEAD
#define UT_FEATURE_EXACT_MODIFIER_HEAD

#include <string>
#include <set>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
class SynNode;


/* 
 *
 * ported from features.py:exact_modifier_head
 */
class UTFeatureExactModifierHead : public UTFeatureType {

public:
   UTFeatureExactModifierHead() : UTFeatureType(Symbol(L"UTFeatureExactModifierHead")) {}


   void modifier_heads(const SynNode &node, set<Symbol> &mods) const {
	  /* only consider children before the head */
	  for (int i = 0 ; i < node.getNChildren() && i < node.getHeadIndex() ; i++) {
		 const SynNode *head = node.getChild(i)->getHeadPreterm();
		 if (head->getTag() != Symbol(L"DT") && 
			 head->getTag() != Symbol(L"WDT")) 
		 {
			Symbol headword = head->getSingleWord();
			std::wstring str_headword = headword.to_string();
			if (!(str_headword[0] == 'm' && str_headword[1] == 'r')) { // starts with mr
			   mods.insert(headword);
			}
		 }
	  }
   }

   /*
     modifiers: all non-head children occuring before the head
	 modifier heads: the head words of the modifiers

	 do a bag of words comparison between the modifier heads of the
	 two nodes, ignoring DT, WDT, and Mr heads.
   */
   Symbol detect(const SynNode &leftNode, const SynNode &rightNode, const LinkAllMentions &lam) const {
	  set<Symbol> lheads;
	  set<Symbol> rheads;

	  modifier_heads(leftNode, lheads);
	  modifier_heads(rightNode, rheads);

	  return (lheads == rheads && lheads.size() > 0) ? True : False;
   }
};

#endif
