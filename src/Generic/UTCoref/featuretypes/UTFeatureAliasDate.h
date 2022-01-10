// Copyright (c) 2011 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef UT_FEATURE_ALIAS_DATE
#define UT_FEATURE_ALIAS_DATE

#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include "Generic/theories/SynNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/UTCoref/LinkAllMentions.h"
#include "Generic/UTCoref/featuretypes/UTFeatureType.h"

class SynNode;

/*
 *
 * ported from features.py:alias_date
 */
class UTFeatureAliasDate : public UTFeatureType {


public:

   /* calculate day, month, and year from n
	  if unknown, -1.  If not a date then -1 for all values.
	  
	  first we find what to consider: we take all the tokens (which
	  are lowercase) and tokenize on sep (which is - and /).  For each
	  token, we see if it's useful for filling m, d, and y in that
	  order.  There are two exceptions to this order: large numbers
	  that could only be years are interpreted as such.  Words that
	  begin with the three letter month abbreviations are considered
	  month designators.

	  Non numeric string portions are ignored.

	  Excess numbers or out of range numbers trigger failure (yielding
	  all -1s)

   */
   
   void interpret(const SynNode &n, int &d, int &m, int &y) const {
	  static const std::wstring jan(L"jan");
	  static const std::wstring feb(L"feb");
	  static const std::wstring mar(L"mar");
	  static const std::wstring apr(L"apr");
	  static const std::wstring may(L"may");
	  static const std::wstring jun(L"jun");
	  static const std::wstring jul(L"jul");
	  static const std::wstring aug(L"aug");
	  static const std::wstring sep(L"sep");
	  static const std::wstring oct(L"oct");
	  static const std::wstring nov(L"nov");
	  static const std::wstring dec(L"dec");
	  
	  static const int jan_n = 1;
	  static const int feb_n = 2;
	  static const int mar_n = 3;
	  static const int apr_n = 4;
	  static const int may_n = 5;
	  static const int jun_n = 6;
	  static const int jul_n = 7;
	  static const int aug_n = 8;
	  static const int sep_n = 9;
	  static const int oct_n = 11;
	  static const int nov_n = 11;
	  static const int dec_n = 12;
	  
	  static boost::char_separator<wchar_t> div(L"-/");

	  d = m = y = -1;

	  std::vector<const SynNode*> terminalNodes;
	  n.getAllTerminalNodes(terminalNodes);
	  BOOST_FOREACH(const SynNode *node, terminalNodes) {
		 const std::wstring node_str(node->getTag().to_string());

		 boost::tokenizer<boost::char_separator<wchar_t>, std::wstring::const_iterator, std::wstring > tokens(node_str, div);
		 BOOST_FOREACH(const std::wstring &str, tokens) {
			int num;
			try {
			   num = boost::lexical_cast<int>(str);
			}
			catch (boost::bad_lexical_cast) {
			   num = -1;
			}
			if (num == -1 && str.length() > 3) {
			   const std::wstring month(str.substr(0, 3));
			   if (month == jan) num = jan_n;
			   else if (month == feb) num = feb_n;
			   else if (month == mar) num = mar_n;
			   else if (month == apr) num = apr_n;
			   else if (month == may) num = may_n;
			   else if (month == jun) num = jun_n;
			   else if (month == jul) num = jul_n;
			   else if (month == aug) num = aug_n;
			   else if (month == sep) num = sep_n;
			   else if (month == oct) num = oct_n;
			   else if (month == nov) num = nov_n;
			   else if (month == dec) num = dec_n;

			   if (num != -1) { // found a month designator
				  m = num;
				  continue;
			   }
			}

			if (num != -1) {
			   // now we have a number to work with

			   if (num > 1000 || (num <= 99 && num > 31)) {
				  if (y == -1) {
					 y = num;
				  }
				  // otherwise ignore this number as out of range
			   }
			   else if (m == -1) {
				  if (num >= 1 && num <= 12) {
					 m = num;
				  }
				  else {
					 d = m = y = -1;
					 return; // fail on bad month
				  }
			   }
			   else if (d == -1) {
				  if (num >= 1 && num <= 31) {
					 d = num;
				  }
				  else {
					 d = m = y = -1;
                     return; // fail on bad day
                  }
			   }
			   else if (y == -1) {
				  if (num >= 0 && num <= 99 && str.length() == 2) {
					 y = num; // good two digit year
				  }
				  else {
					 d = m = y = -1;
                     return; // fail on bad year
                  }
			   }
			   else {
				  d = m = y = -1;
				  return; // fail because we have too many numbers
			   }
			}
		 }
	  }
   }

   bool check(int i_d, int i_m, int i_y, 
			  int j_d, int j_m, int j_y) const
   {
	  if (i_d != j_d || i_m != j_m) {
		 return false; // day and month must match
	  }
	  if ((i_d == -1 && i_m == -1 && i_y == -1) ||
		  (j_d == -1 && j_m == -1 && j_y == -1))
	  {
		 return false; // not recognizable as a date
	  }
	  if (i_y != -1 && j_y != -1 && i_y != j_y &&
		  !((i_y < 100 || j_y < 100) && i_y%100 == j_y%100))  // deal with short formats
	  {
		 return false; // years have to match if specified.
	  }

	  return true;
   }

   Symbol detect(const SynNode &leftNode,
				 const SynNode &rightNode,
				 const LinkAllMentions &lam) const {
	  
	  static Symbol sDATE = Symbol(L"DATEe");
	  
	  if (lam.getName(leftNode) != sDATE ||
		  lam.getName(rightNode) != sDATE)
	  {
		 return False;
	  }

	  int l_d, l_m, l_y, r_d, r_m, r_y;
	  interpret(leftNode, l_d, l_m, l_y);
	  interpret(rightNode, r_d, r_m, r_y);

	  return (check(l_d, l_m, l_y, r_d, r_m, r_y) || check(r_d, r_m, r_y, l_d, l_m, l_y)) ? True : False;
   }

   UTFeatureAliasDate() : UTFeatureType(Symbol(L"UTFeatureAliasDate")) {}
};
#endif
