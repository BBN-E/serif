// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Preprocessor/Rule.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"

Rule::Rule(int id, const std::wstring regEx, const std::wstring action1, 
		   const std::wstring action2, const int subExp, const ActionType type)
	: _id(id), _regex_str(regEx), _pre_action(action1), 
	_post_action(action2), _sub_exp(subExp), _typeAction(type)
{
	//set actions depending on type of action
	if(type == PRE){
		_pre_action = action1;
		_post_action = L"";
	}
	else if (type == POST){
		_pre_action = L"";
		_post_action = action1;
	}
	else if (type == BOTH){
		_pre_action = action1;
		_post_action = action2;
	}
	else {
		char message[500];
		sprintf(message, "Rule %d: Invalid action type.\n", _id);
		throw InternalInconsistencyException("Rule::Rule()", message);
	}
}

Rule::Rule(const Rule& otherRule) {
	_id = otherRule._id;
	_regex_str = otherRule._regex_str;
	_pre_action = otherRule._pre_action;
	_post_action = otherRule._post_action;
	_regex = otherRule._regex;
	_sub_exp = otherRule._sub_exp;
	_typeAction = otherRule._typeAction;
}

void Rule::validateRegEx(){
	//std::wcout<<"In validate RegEx(): "<<_regex_str<<"\n";
	try {
		_regex.assign(_regex_str);
	} catch (boost::regex_error& e){
		char message[500];
		sprintf(message, "Rule %d: Invalid regular expression: %s\n",_id, e.what());
		throw UnexpectedInputException("RuleList::validateRule()", message);
	}
}

void Rule::validateSubEx(){
	if (_sub_exp < 0) {
		char message[500];
		sprintf(message, "Rule %d: Subexpressions cannot be negative.\n",_id);
		throw UnexpectedInputException("Rule::validateSubEx()", 
									   message);
	}
}

void Rule::validateActions(){
	validateAction(_pre_action);
	validateAction(_post_action);
}

void Rule::validateAction(std::wstring action){
	bool inTag = false;
	for(size_t i=0; i< action.size(); i++){
		if (inTag){
			if (action.at(i) == '>')
				inTag = false;
			else if (action.at(i) == '<'){
				char message[500];
				sprintf(message, "Rule %d: Can not have '<' inside an XML tag\n",_id);
				throw UnexpectedInputException("Rule::validateAction()",
											   message);
			}
		}
		else{
			if(action.at(i) == '<')
				inTag = true;
			else{
				char message[500];
				sprintf(message, "Rule %d: Everything that is inserted must be inside XML style brackets ('<' '>')\n",_id);
				throw UnexpectedInputException("Rule::validateAction()", message); 
			}
		}
	}
}

void Rule::validateRule(){
	validateRegEx();
	validateSubEx();
	validateActions();
}
