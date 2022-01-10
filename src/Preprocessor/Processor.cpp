// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Preprocessor/Processor.h"
#include "Preprocessor/RegexFind.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/ParamReader.h"
#include <boost/scoped_ptr.hpp>

Processor::Processor() {
	char buf[4096];

	ParamReader::getRequiredParam("zoning_rule_file", buf, 4096);
	boost::scoped_ptr<UTF8InputStream> ruleStream_scoped_ptr(UTF8InputStream::build(buf));
	UTF8InputStream& ruleStream(*ruleStream_scoped_ptr);
	if (ruleStream.fail())  {
		throw UnexpectedInputException("Processor::Processor()",
									   "Could not open Rule File.");
	}
	_rules.readRules(ruleStream);
	ruleStream.close();
}

Processor::Processor(const char *rule_file) {
	// Load the zoning rules
	boost::scoped_ptr<UTF8InputStream> ruleStream_scoped_ptr(UTF8InputStream::build(rule_file));
	UTF8InputStream& ruleStream(*ruleStream_scoped_ptr);
	if (ruleStream.fail())  {
		throw UnexpectedInputException("Processor::Processor()",
									   "Could not open Rule File.");
	}
	_rules.readRules(ruleStream);
	ruleStream.close();
}

/* Load the data file into a LocatedString 
	return the new located string
	caller is responsible for deleting string
*/
LocatedString* Processor::loadData(const std::wstring data_file_name){
	
	//open data file
	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build(data_file_name.c_str()));
	UTF8InputStream& in(*in_scoped_ptr);
	if(in.fail()){
		char message[500];
		sprintf(message, "Failed to open `%s'", data_file_name.c_str());
		throw UnexpectedInputException("Processor::loadData()", message);
	}
	
	LocatedString *dataStr = new LocatedString(in, true);	
	return dataStr;
}
/*
	Finds the regular expressions in the string and inserts the proper tags
	around the specified substring. 
*/
void Processor::processFile(const std::wstring data_file_name, 
							const std::wstring out_file_name)
{
	std::wcout<<"Processing "<<data_file_name<<std::endl;
	(*SessionLogger::logger)<<"Processing "<<data_file_name<<":\n";

	LocatedString *dataStr = loadData(data_file_name);

	processLocatedString(dataStr);
	
	writeData(dataStr, out_file_name);
	delete dataStr;
}

/*
	Finds the regular expressions in the string and inserts the proper tags
	around the specified substring. 
*/
void Processor::processLocatedString(LocatedString *string) {

	int num_rules = _rules.getNumRules();
	
	RegexFind finder(_rules);
	for (int i = 0; i < num_rules; i++) {
		(*SessionLogger::logger)<<"Rule "<<i<<":\n";
		
		if (finder.findExpression(string->toString(), i)) {
			(*SessionLogger::logger)<<"\t Matches: "<<finder.getNumMatches(i)<<"\n";
			makeInsertions(string, finder.getMatches(i), i);
		}
		else
			(*SessionLogger::logger)<<"\t Matches: 0\n";		
	}
}

void Processor::makeInsertions(LocatedString *dataStr, std::vector<RegexMatch> matches, int rule_index){
	
	Rule currRule = _rules.getNthRule(rule_index);
	std::wstring pre_action =currRule.getPreAction();
	int pre_action_len = (int)wcslen(pre_action.c_str());
	std::wstring post_action = currRule.getPostAction();
	int post_action_len = (int)wcslen(post_action.c_str());

	std::vector<int> preActions;
	std::vector<int> postActions;
	for (size_t i=0; i< matches.size(); i++){
		int length = matches.at(i).getLength();
		int start_position = matches.at(i).getPosition();
		int end_position = start_position + length;
		if (pre_action_len > 0) 
			preActions.push_back(start_position);
		if (post_action_len > 0)
			postActions.push_back(end_position);
	}

	std::sort(preActions.begin(), preActions.end());
	std::sort(postActions.begin(), postActions.end());

	while (!preActions.empty() && !postActions.empty()) {
		if (postActions.back() >= preActions.back()) {
			dataStr->insert(post_action.c_str(), postActions.back());
			postActions.pop_back();
		}
		else {
			dataStr->insert(pre_action.c_str(), preActions.back());
			preActions.pop_back();
		}
	}

	while (!preActions.empty()) {
		dataStr->insert(pre_action.c_str(), preActions.back());
		preActions.pop_back();
	}

	while (!postActions.empty()) {
		dataStr->insert(post_action.c_str(), postActions.back());
		postActions.pop_back();
	}
}

void Processor::writeData(LocatedString *dataStr, const std::wstring out_file_name){
	UTF8OutputStream out(out_file_name.c_str());
	out.write(dataStr->toString(), wcslen(dataStr->toString()));
	out.close();
}
