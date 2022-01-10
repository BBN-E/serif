// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Preprocessor/RegexFind.h"
#include "Generic/common/InternalInconsistencyException.h"

bool RegexFind::findExpression(const wchar_t *data_string, int ruleIndex){
	
	if (ruleIndex < 0 || (size_t)ruleIndex >= _expressions.size()) {
		throw InternalInconsistencyException::arrayIndexException(
											"RegexFind::findExpression()",
											(int)_expressions.size(), ruleIndex);
					
	}

	return apply_expression(data_string, ruleIndex);
}


int RegexFind::getNumMatches(int ruleIndex)const {
	if(ruleIndex >= int(_matches.size())) {
		throw InternalInconsistencyException::arrayIndexException(
											"RegexFind::getNumMatches()",
											(int)_matches.size(), ruleIndex);
	}
	return int(_matches.at(ruleIndex).size());
}

const std::vector<RegexMatch>& RegexFind::getMatches(int ruleIndex)const {
	if(ruleIndex >= int(_matches.size())) {
		throw InternalInconsistencyException::arrayIndexException(
											"RegexFind::getMatches()",
											(int)_matches.size(), ruleIndex);
	}
	return _matches.at(ruleIndex);
}

bool RegexFind::apply_expression(const wchar_t *data_string, int index) {
	//std::wcout<<"Rule: "<<_rules.getNthRule(index).getPattern()<<std::endl;
	boost::wregex expression = _expressions.at(index);
	int sub_exp = _rules.getNthRule(index).getSubExprNum();
	std::wstring lstext = data_string;

	//std::cout<<"Constructing...\n";
    // construct our iterators:
    boost::wsregex_iterator regex_it(lstext.begin(), lstext.end(), expression);
    boost::wsregex_iterator regex_end;

    std::vector<RegexMatch> matches;

	if (sub_exp < 0 || (regex_it != regex_end && (size_t)sub_exp >= regex_it->size())) {
		char message[500];
		sprintf(message, "Invalid subexpression index for rule %d.\n", index);
		throw UnexpectedInputException("RegexFind::apply_expression()", message);
	}

	if (regex_it == regex_end) {
		_matches.push_back(matches); // push empty vector onto match list
		return false;
	}

	std::for_each(regex_it, regex_end, boost::bind(&regex_callback, _1, sub_exp, &matches));
   	
	_matches.push_back(matches);

	return true;
}

void regex_callback(const boost::wsmatch& what,           // input
                    int sub_string_pos,               // input
					std::vector<RegexMatch>* matches_p)   // output (notice its a ptr!)
{
	// what[0] contains the whole string
	// add found substring, position, and length to matches

	
	int position = (int)what.position(sub_string_pos);
	int length   = (int)what[sub_string_pos].str().length();
	std::wstring value = what[sub_string_pos].str();
	//std::wcout<<value<<"\t"<<position<<"\t"<<length<<endl;
	RegexMatch curr_match (value, position, length);
	matches_p->push_back(curr_match);
	
}

void print_match_callback(const RegexMatch& match)
{
  std::wcout << "Found value: " 
            << match.getString() 
            << " at position: " 
			<< match.getPosition() 
            << " with length: " 
            << match.getLength() 
            << "." 
            << std::endl;
}
