// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PREPROCESSOR_PROCESSOR_H
#define PREPROCESSOR_PROCESSOR_H

#include "Preprocessor/RuleList.h"
#include "Generic/common/RegexMatch.h"
#include "Generic/common/LocatedString.h"

class Processor {
	public:
		Processor(); 
		Processor(const char *rule_file);

		void processLocatedString(LocatedString *string);
		void processFile(const std::wstring data_file_name, const std::wstring out_file_name);
		
	private:
		RuleList _rules;

		LocatedString* loadData(const std::wstring data_file_name);
		void writeData(LocatedString *dataStr, const std::wstring out_file_name);
		void makeInsertions(LocatedString *dataStr, 
							std::vector<RegexMatch> matches, 
							int rule_index);
};
#endif
