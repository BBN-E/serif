// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <iostream>
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/FileSessionLogger.h"
#include "Preprocessor/RuleList.h"
#include "Preprocessor/Processor.h"

#include "Generic/common/version.h"
#include <boost/scoped_ptr.hpp>

//This program acts as a Serif preprocessor and uses regular expressions to label 
// regions of text.


void PrintUsageMessage();


int main(int argc, const char** argv)
{
	if (argc != 2) {
		PrintUsageMessage();
		return -1;
	}
	try {
		ParamReader::readParamFile(argv[1]);

		char rule_file[501];
		char file_list[501];
		char output_dir[501];
		wchar_t wide_output_dir[501];

		if (!ParamReader::getParam("rule_file",rule_file, 500))		{
			throw UnexpectedInputException("Preprocessor::main()",
									   "Param `rule_file' not defined");
		}
		if (!ParamReader::getParam("preprocessor_file_list",file_list, 500))		{
			throw UnexpectedInputException("Preprocessor::main()",
									   "Param `preprocessor_file_list' not defined");
		}
		if (!ParamReader::getParam("output_dir",output_dir, 500))		{
			throw UnexpectedInputException("Preprocessor::main()",
									   "Param `output_dir' not defined");
		}
		
		// change the output_dir to a wide character string
		mbstowcs(wide_output_dir, output_dir, 500);

		std::string log_file = ParamReader::getRequiredParam("log_file");
		std::wstring log_file_as_wstring(log_file.begin(), log_file.end());

		SessionLogger::logger = new FileSessionLogger(log_file_as_wstring.c_str(), 0, 0);
		SessionLogger::logger->beginMessage();
		*SessionLogger::logger << "This is the Serif Preprocessor.\n"
						       << "Serif version: " << SerifVersion::getVersionString() << "\n"
							   << "Serif Language Module: " << SerifVersion::getSerifLanguage().toString() << "\n"
							   << "\n";
		*SessionLogger::logger << "_________________________________________________\n"
							   << "Starting Preprocessor Session\n"
							   << "Parameters:\n";

		ParamReader::logParams();
		*SessionLogger::logger << "\n";

		Processor processor(rule_file);

		boost::scoped_ptr<UTF8InputStream> fileListStream_scoped_ptr(UTF8InputStream::build(file_list));
		UTF8InputStream& fileListStream(*fileListStream_scoped_ptr);
		std::wstring in_data_file;
		std::wstring base_filename;
		std::wstring out_file;
		UTF8Token token;
		
		while (!fileListStream.eof()) {

			fileListStream >> token;
			
			if (wcscmp(token.chars(), L"") == 0)
				break;

			//Get Data input and output file names
			in_data_file = token.chars();
			
			base_filename = std::wstring(L"");
			size_t slash = in_data_file.find_last_of(L'\\');
			if (slash == wstring::npos) {
				slash = in_data_file.find_last_of(L'/');
			}
			if (slash != wstring::npos)
				base_filename = in_data_file.substr(slash+1);

			out_file = std::wstring(wide_output_dir);
			wchar_t last_char = out_file.at(out_file.length()-1); 
			if (last_char != L'/' && last_char != L'\\') {
				out_file.append(L"/");
			}
			out_file.append(base_filename);

			//Process data file
			processor.processFile(in_data_file, out_file);
		}

	} catch (UnrecoverableException &e) {

		e.putMessage(std::cerr);
		return -1;
	}

	return 0; 
}


void PrintUsageMessage(){
	std::cout<<"Usage: PreProcessor.exe <Parameter File>\n";
	std::cout<<"\n\nThe Parameter File should contain the following parameters:\n"
			 <<"rule-file, preprocessor-file-list, log-file\n";
	std::cout<<"\n\nThe Rule file should be in the form of:\n"
		<<"# of rules\n\nRegular Expression 1\n# of subexpression\nPRE|POST|BOTH\nXML Tags to insert\n\n ...\n\nRegular Expression n\n# of subexpression\nPRE|POST|BOTH\nXML Tags to insert\n"
		<<"\n Make sure you have a blank line after each rule/action set and also after the number of rules.\n";
}
