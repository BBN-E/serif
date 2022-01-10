// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Preprocessor/RuleList.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/SessionLogger.h"



/*Reads in rules from a rule file of the form:
	n
	rule 1
	[PRE/POST/BOTH]
	action 1  
	action 1(only if BOTH)
	
	rule n
	[PRE/POST/BOTH]
	action n
*/
void RuleList::readRules(UTF8InputStream& in){
	
	//get number of Rules
	in>>_num_rules;
	
		
	if(_num_rules <= 0)
		throw UnexpectedInputException("RuleList::readRules()", "Invalid number of rules.");
	
	
	//get end of line after numRules
	std::wstring temp;
	in.getLine(temp);
	//std::cout<<_num_rules<<std::endl;

	//Get all rules
	for(int i=0; i<_num_rules; i++){
		//get blank line before each rule/action set
		in.getLine(temp);
		if(!temp.empty()){
			throw UnexpectedInputException("RuleList::readRules()", 
						"Rule file must contain a blank line after the number of rules and after each rule set.");
		}
	
		//get Regular Expression
		std::wstring reg_ex;
		if(!in.eof())
			in.getLine(reg_ex);
		else{
			char message[500];
			sprintf(message, "Reached end of file before all %d rules were read.\n", _num_rules);
			throw UnexpectedInputException("RuleList::readRules()", message);
		}
		//std::wcout<<"RegEx = "<<reg_ex<<std::endl;
	
		//get subexpression number
		int sub_exp;
		in>>sub_exp;
		
		//get end of line after subExp
		in.getLine(temp);

		//get Action Position (PRE POST BOTH)
		std::wstring action_type;
		ActionType type;
		if(!in.eof())
			in.getLine(action_type);
		else{
			char message[500];
			sprintf(message, "Reached end of file before all %d rules were read.\n", _num_rules);
			throw UnexpectedInputException("RuleList::readRules()", message);
		}
		if(action_type == L"PRE")
			type = PRE;
		else if(action_type == L"POST")
			type = POST;
		else if (action_type == L"BOTH")
			type = BOTH;
		else{
			char message[500];
			sprintf(message, "Rule %d: Invalid action type. Action type must be PRE, POST or BOTH.\n", i);
			throw UnexpectedInputException("RuleList::readRules()", message);
		}
		//std::wcout<<"Action Type: "<<action_type<<std::endl;
	
		//get Action
		std::wstring action1,action2;
		if(!in.eof())
			in.getLine(action1);
		else{
			char message[500];
			sprintf(message, "Reached end of file before all %d rules were read.\n", _num_rules);
			throw UnexpectedInputException("RuleList::readRules()", message);
		}
		//std::wcout<<"Action = "<<action1<<std::endl;

		//get 2nd Action if actionType = BOTH
		if(type == BOTH){
			if(!in.eof())
				in.getLine(action2);
			else{
				char message[500];
				sprintf(message, "Reached end of file before all %d rules were read.\n", _num_rules);
				throw UnexpectedInputException("RuleList::readRules()", message);
			}	

			if(action2.empty()){
				throw UnexpectedInputException("RuleList::readRules()", "2 actions should be given if the type is BOTH.");
			}
		}
		
		//add rule to RuleList
		Rule tempRule(i, reg_ex, action1, action2, sub_exp,type);

		tempRule.validateRule();
		_rules.push_back(tempRule);
		
	}
	while(!in.eof()){
		in.getLine(temp);
		if(!temp.empty()){
			std::cerr << "There may be more than "<<_num_rules<<" rule(s) in your rule file. Executing with "<<_num_rules<<" rule(s).\n";
			(*SessionLogger::logger)<<"There may be more than "<<_num_rules<<" rule(s) in your rule file. Executing with "<<_num_rules<<" rule(s).\n";
			break;
		}
	}
	
}

const Rule& RuleList::getNthRule(const int n)const {
	return _rules.at(n);
}

