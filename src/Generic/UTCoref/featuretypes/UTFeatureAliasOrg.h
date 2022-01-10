// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_ORG
#define UT_FEATURE_ALIAS_ORG

#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"
#include "Generic/UTCoref/featuretypes/UTFeatureAOfBIsBA.h"
class SynNode;


/*
 *
 * ported from features.py:alias_org
 */
class UTFeatureAliasOrg : public UTFeatureType {

public:
  UTFeatureAliasOrg() : UTFeatureType(Symbol(L"UTFeatureAliasOrg")) {

	 aofbisba = static_cast<UTFeatureAOfBIsBA*>(
		getFeatureType(getModel(), Symbol(L"UTFeatureAOfBIsBA")));
  }

private:

  UTFeatureAOfBIsBA *aofbisba;

  void getNonATheOf(const SynNode &n, const LinkAllMentions &lam, std::wstring &n_str) const {
	 static Symbol sA(L"a");
	 static Symbol sAn(L"an");
	 static Symbol sThe(L"the");
	 static Symbol sOf(L"of");

	 std::vector<const SynNode*> terminals;
	 n.getAllTerminalNodes(terminals);

	 BOOST_FOREACH(const SynNode* terminal, terminals) {
		Symbol tag = terminal->getTag();
		if (tag != sA && tag != sAn && tag != sThe && tag != sOf) {
		   n_str += std::wstring(lam.casedString(*terminal->getParent()));
		   n_str += L" ";
		}
	 }
  }

  // returns std::wstring("") on failure
  std::wstring acro(const SynNode &n, const LinkAllMentions &lam,
					bool ignore_lower, bool periods, bool delpomos) const {

	 std::vector<const SynNode*> terminals;
	 n.getAllTerminalNodes(terminals);

	 std::wstring acr;

	 BOOST_FOREACH(const SynNode* terminal, terminals) {
		Symbol tag = terminal->getTag();
		if (!delpomos || !lam.is_premodifier(tag)) { // exclude premodifiers
		   std::wstring s = lam.casedString(*terminal);

		   if (s.length() > 0 && ((s[0] >= L'A' && s[0] <= L'Z') || !ignore_lower)) {
			  acr += s[0];
			  if (periods) {
				 acr += L".";
			  }
		   }
		}
	 }

	 if (acr.length() < 2) { // too short
		return std::wstring(L""); // wont match anything in the compare later
	 }
	 return acr;
  }

public:

  bool detectHelper(const SynNode &a,
					const SynNode &b,
					const LinkAllMentions &lam,
					bool gpe_is_org, /* treat gpes as orgs */
					bool ignore_lower, /* ignore lowercase words in making an acronym */
					bool of_alias, /* call UTFeatureAOfBIsBA if the name types are good */
					bool org_match_first_word) const { /* see comment below */

	 static Symbol sA(L"a");
	 static Symbol sAn(L"an");
	 static Symbol sThe(L"the");

	 Symbol a_name = lam.getName(a);
	 Symbol b_name = lam.getName(b);

	 if (a_name != b_name) {
		return false;
	 }

	 if (a_name != lam.org && !(gpe_is_org && a_name == lam.gpe)) {
		return false;
	 }

	 if (of_alias && aofbisba->detect(a,b,lam) == True) {
		return true;
	 }

	 /* paying attention only to capitalized words, whether the string
	    for a is identical to the beginning of the string for b
	    (ignoring 'a', 'the', and 'of') */
	 if (org_match_first_word) {
		std::wstring a_str, b_str;
		getNonATheOf(a, lam, a_str);
		getNonATheOf(b, lam, b_str);

		if (a_str.length() > b_str.length()) {
		   return false;
		}

		if (a_str == b_str.substr(0, a_str.length())) {
		   return true;
		}
	 }

	 /* now check for acronym identity */
	 std::wstring a_str = lam.casedString(a);
	 if (a.getNTerminals() != 1) {
		if (a.getNTerminals() != 2) {
		   return false;
		}

		Symbol tag = a.getNthTerminal(0)->getTag();

		if (tag != sA && tag != sAn && tag != sThe) {
		   return false;
		}

		a_str = lam.casedString(*a.getNthTerminal(1));
	 }


	 if (a_str == acro(b, lam, ignore_lower, true, true) ||
		 a_str == acro(b, lam, ignore_lower, true, false) ||
		 a_str == acro(b, lam, ignore_lower, false, false) ||
		 a_str == acro(b, lam, ignore_lower, false, true))
	 {
		return true;
	 }

	 return false;
  }

  Symbol detect(const SynNode &leftNode,
                const SynNode &rightNode,
                const LinkAllMentions &lam) const {
	 return detect(leftNode, rightNode, lam, false, false, false, false);
  }

  Symbol detect(const SynNode &leftNode,
				const SynNode &rightNode,
				const LinkAllMentions &lam,
				bool gpe_is_org=false,
				bool ignore_lower=false,
				bool of_alias=false,
				bool org_match_first_word=false) const
  {
	 return (detectHelper(leftNode, rightNode, lam, gpe_is_org, ignore_lower, of_alias, org_match_first_word) ||
			 detectHelper(rightNode, leftNode, lam, gpe_is_org, ignore_lower, of_alias, org_match_first_word) )
		? True : False;
  }
};

#endif
