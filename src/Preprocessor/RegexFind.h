// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREPROCESSOR_REGEXFIND_H
#define PREPROCESSOR_REGEXFIND_H

#include "Generic/common/LocatedString.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/RegexMatch.h"
#include "Preprocessor/RuleList.h"
#include "Preprocessor/Rule.h"

#include <boost/regex.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>

#include <vector>


class RegexFind{
public:
	RegexFind(const RuleList& rules)
		:_rules(rules)
	{
		for(int i =0; i<rules.getNumRules(); i++)
		{
			_expressions.push_back(rules.getNthRule(i).getRegEx());
		}
	}

	bool findExpression(const wchar_t *data_string, int ruleIndex);
	int getNumMatches(int ruleIndex)const;
	const std::vector<RegexMatch>& getMatches(int ruleIndex)const;
private:
	RuleList _rules;
	std::vector<std::wstring> _str_expressions;
	std::vector<boost::wregex> _expressions;
	std::vector< std::vector<RegexMatch> > _matches;
	bool apply_expression(const wchar_t *data_string, int index);
	
	bool isValidSubstring(const boost::wsmatch& what, int sub_string_pos);

};

void regex_callback(const boost::wsmatch& what, int sub_string_pos, std::vector<RegexMatch>* matches_p); 
void print_match_callback(const RegexMatch& match);

#endif

