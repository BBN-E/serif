// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREPROCESSOR_RULELIST_H
#define PREPROCESSOR_RULELIST_H

#include <iostream>
#include <vector>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Preprocessor/Rule.h"



class RuleList{
	private:
		std::vector<Rule> _rules;
		int _num_rules;
	
	public:
		RuleList(){
			_num_rules = 0;
		}
		void readRules(UTF8InputStream& in);
		int getNumRules() const {return _num_rules;}
		const Rule& getNthRule(const int n)const ;
};

#endif
