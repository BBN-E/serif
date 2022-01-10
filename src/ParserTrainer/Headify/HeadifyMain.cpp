// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/UTF8InputStream.h"
#include "common/UTF8OutputStream.h"
#include "common/UnexpectedInputException.h"
#include <cstring>
#include "common/UTF8Token.h"
#include "common/Symbol.h"
#include "parse/ParserTrainer/HeadlessParseNode.h"
#include "common/ParamReader.h"
#include "trainers/HeadFinder.h"
#include "Generic/common/FeatureModule.h"
#include <boost/scoped_ptr.hpp>
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"


// DIVERSITY CHANGES:
// For data w/o named entities, Headify will run exactly as
// the regular parser's did. For data w/ named entities, it will
// replace PERSON/ORGANIZATION/LOCATION with NPP, and change their
// children's tags from NNP/NNPS to NPP-NNP/NPP-NNPS
		

int main(int argc, char* argv[]) {
	UTF8Token token;
	HeadlessParseNode* parse;
	
	if ((argc != 4) && (argc != 5) ) {
		std::cerr << "wrong number of arguments to headifier\n";
		std::cerr << "Usage: input_file output_file language [parameter_file]\n\n";
		std::cerr << "(language parameter is new: it should be English, Arabic, etc)\n";
		return -1;
	}
	
	try {
    
		// Load the specified language feature module.
		FeatureModule::load(argv[3]);

		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(argv[1]);
		
		UTF8OutputStream out;
		out.open(argv[2]);
		//NOTE this is different than the original Parser Trainer
		//The Chinese HeadFinder loads a table and is not static.  
		//Make the headfinder an object that is passed to all nodes
		//If a parmeter file argument is included, read the parameter file
		//It will be used by the Chinese HeadFinder
		if(argc == 5){
			ParamReader::readParamFile(argv[4]);
		}
		ParserTrainerLanguageSpecificFunctions::initialize();
		HeadFinder * _headFinder = HeadFinder::build();
		
		Symbol top = Symbol(L"TOP");
		Symbol top_tag = Symbol(L"TOPTAG");
		Symbol top_word = Symbol(L"TOPWORD");
		Symbol empty_parse = Symbol(L"EMPTY_PARSE");

		int line_no = 0;
		
		while (true) {
			
			line_no++;

			in >> token;
			if (in.eof())
				break;
			if (wcscmp(token.chars(), L"(") != 0) {
				std::cerr << "On line " << line_no << std::endl;
				for (int i=0; i<20; ++i) {
					std::wcout << "[" << token.chars() << "] ";
					in >> token;
				}
				std::wcout << std::endl;
				throw UnexpectedInputException("HeadifyMain::main()",
				"ERROR: ill-formed sentence in input file\n");
			}
			
			// (TOP (TOPTAG TOPWORD) (S ... ))
			parse = new HeadlessParseNode(top, _headFinder);
			parse->isPreTerminal = false;
			parse->children = new HeadlessParseNode(top_tag, _headFinder);
			parse->children->isPreTerminal = true;
			parse->children->children = new HeadlessParseNode(top_word, _headFinder);
			parse->children->next = new HeadlessParseNode(_headFinder);
			parse->children->next->read(in);
			
			if (parse->children->next->label != empty_parse) {
				try {
					out << parse->Headify_to_string();
					out << L"\n";
				} catch (UnexpectedInputException uc) {
					std::cerr << "Line " << line_no << " ";
					uc.putMessage(std::cerr);
				}
			}
			
			delete parse;
			
		}
		
    in.close();
    out.close();
		
	} catch (UnexpectedInputException uc) {
		uc.putMessage(std::cerr);
		return -1;
		
	} 
	
	
	return 0;
}
