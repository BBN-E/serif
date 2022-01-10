// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREPROCESSOR_RULE_H
#define PREPROCESSOR_RULE_H

#include <iostream>
#include <boost/regex.hpp>

enum ActionType {PRE, POST, BOTH};


class Rule{
private:
		int _id;
		std::wstring _regex_str;
		std::wstring _pre_action;
		std::wstring _post_action;
		boost::wregex _regex;
		int _sub_exp;
		ActionType _typeAction;

		void validateRegEx();
		void validateSubEx();
		void validateActions();
		void validateAction(std::wstring action);

public:
		
		Rule(int id, const std::wstring regEx, const std::wstring action1, 
			 const std::wstring action2, const int subExp, const ActionType type);
				
		Rule (const Rule& otherRule);				
		boost::wregex getRegEx() const {return _regex;}
		std::wstring getPattern() const {return _regex_str;}
		std::wstring getPreAction() const {return _pre_action;}
		std::wstring getPostAction() const{return _post_action;}
		int getSubExprNum()const{return _sub_exp;}
		ActionType getActionType() const{return _typeAction;}
		void validateRule();
};

#endif
