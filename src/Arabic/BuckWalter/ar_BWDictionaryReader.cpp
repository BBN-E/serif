// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Arabic/BuckWalter/ar_BWDictionaryReader.h"
#include "Generic/common/UTF8InputStream.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"

#include <string>
#include <string.h>
#include <iostream>
#include <wchar.h>
#include <boost/scoped_ptr.hpp>


void BWDictionaryReader::addToBWDictionary(Lexicon* lex, const char *bw_dict_file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(bw_dict_file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	LexicalEntry* analysis[30];
	size_t numEntries;
	size_t ID;
    UTF8Token token;
	Symbol keyWithoutVowels;
	Symbol category;
	Symbol keyWithVowels;
	Symbol partOfSpeech;
	int num_sub_items;
	wchar_t buf[1000];
	size_t id;
	Symbol gloss;

	stream >> numEntries;
	if (stream.fail()) {
		// Make sure the dictionary file is not empty.
		stream.close();
		return;
	}

	for(size_t line = 0; line < numEntries; line++){


		//The first field is the ID number
		stream >> ID;
		if(ID != (line+lex->getStartSize())) {
			char message[1000];
			sprintf(message, "%s%lu%s%lu%s%lu%s%s", "entry line number, ", line, 
				 " ,start size" ,lex->getStartSize()  ,
				", do not match ID number, ", ID, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
		}


		//The next token is the unvowelled key
		stream >> token;
		keyWithoutVowels = token.symValue();


		//The next token is the category
		stream >> token;
		category = token.symValue();

		//The next token is the unvowelled key
		stream >> token;
		keyWithVowels = token.symValue();

		//The next token is the POS
		stream >> token;
		partOfSpeech = token.symValue();

		//the next token should be a left paren
		stream >> token;
		if(token.symValue() != Symbol(L"(")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected left paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
		}

		stream >> token;
		buf[0] = L'\0';
		while (token.symValue() != Symbol(L")")){
			wcscat(buf, token.symValue().to_string()); 
			stream >> token;
		}
		gloss = Symbol(buf);

		//There should be a right paren inside of token now
		if(token.symValue() != Symbol(L")")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected right paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
		}
		
		//the next token should be a left paren again
		stream >> token;
		if(token.symValue() != Symbol(L"(")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected left paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
		}
	
		//get the number of items in the analysis
		num_sub_items = 0;
		stream >> num_sub_items;

		//check to make sure they all exist and build the array 
		//that we need to create the LexicalEntry		
		for(int j = 0; j < 30; j++){
			analysis[j] = NULL;
		}

		for(int	item = 0; item < num_sub_items; item++){
			stream >> id;
			if(!(lex->hasID(id))){
				char message[1000];
				sprintf(message, "%s%lu%s%lu%s%s", "Analysis includes a lexical entry not yet in the dictionary: ", id," on line ", line, " in file: ", bw_dict_file);  
				throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
			}else{
				analysis[item] = lex->getEntryByID(id);		
			}						
		}

		stream >> token;
		if(token.symValue() != Symbol(L")")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected right paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ar_BWDictionaryReader::addToBWDictionary", message);
		}

		//create the 
		FeatureValueStructure* fvs = FeatureValueStructure::build(category, keyWithVowels, partOfSpeech, gloss);
		LexicalEntry* le = _new LexicalEntry(ID, keyWithoutVowels, fvs, analysis, num_sub_items);
		lex->addDynamicEntry(le);
	}
	
	
	stream.close();
}


Lexicon* BWDictionaryReader::readBWDictionaryFile(const char *bw_dict_file) {
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(bw_dict_file));
	UTF8InputStream& stream(*stream_scoped_ptr);

	LexicalEntry* analysis[30];
	size_t numEntries;
	size_t ID;
    UTF8Token token;
	Symbol keyWithoutVowels;
	Symbol category;
	Symbol keyWithVowels;
	Symbol partOfSpeech;
	int num_sub_items;
	wchar_t buf[1000];
	size_t id;
	Symbol gloss;

	stream >> numEntries;

	//std::cout<<"readBWDictionaryFile(): make dictionary with "<<static_cast <int>(numEntries)<<" entries"<<std::endl;
	Lexicon* lex = _new Lexicon(numEntries);

	for(size_t line = 0; line < numEntries; line++){


		//The first field is the ID number
		stream >> ID;
		if(ID != line){
			char message[1000];
			sprintf(message, "%s%lu%s%lu%s%s", "entry line number, ", line, ", does not match ID number, ", ID, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("BWDictionaryReader::readBWDictionaryFile", message);
		}


		//The next token is the unvowelled key
		stream >> token;
		keyWithoutVowels = token.symValue();


		//The next token is the category
		stream >> token;
		category = token.symValue();

		//The next token is the unvowelled key
		stream >> token;
		keyWithVowels = token.symValue();

		//The next token is the POS
		stream >> token;
		partOfSpeech = token.symValue();

		//the next token should be a left paren
		stream >> token;
		if(token.symValue() != Symbol(L"(")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected left paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ParamReader::readParamFile", message);
		}

		stream >> token;
		buf[0] = L'\0';
		while (token.symValue() != Symbol(L")")){
			wcscat(buf, token.symValue().to_string()); 
			stream >> token;
		}
		gloss = Symbol(buf);

		//There should be a right paren inside of token now
		if(token.symValue() != Symbol(L")")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected right paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ParamReader::readParamFile", message);
		}
		
		//the next token should be a left paren again
		stream >> token;
		if(token.symValue() != Symbol(L"(")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected left paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ParamReader::readParamFile", message);
		}
	
		//get the number of items in the analysis
		num_sub_items = 0;
		stream >> num_sub_items;

		//check to make sure they all exist and build the array 
		//that we need to create the LexicalEntry		
		for(int j = 0; j < 30; j++){
			analysis[j] = NULL;
		}

		for(int	item = 0; item < num_sub_items; item++){
			stream >> id;
			if(!(lex->hasID(id))){
				char message[1000];
				sprintf(message, "%s%lu%s%lu%s%s", "Analysis includes a lexical entry not yet in the dictionary: ", id," on line ", line, " in file: ", bw_dict_file);  
				throw UnexpectedInputException("ParamReader::readParamFile", message);
			}else{
				analysis[item] = lex->getEntryByID(id);		
			}						
		}

		stream >> token;
		if(token.symValue() != Symbol(L")")){
			char message[1000];
			sprintf(message, "%s%lu%s%s", "expected right paren at line number: ", line, " in file: ", bw_dict_file);  
			throw UnexpectedInputException("ParamReader::readParamFile", message);
		}

		//create the 
		FeatureValueStructure* fvs = FeatureValueStructure::build(category, keyWithVowels, partOfSpeech, gloss);
		LexicalEntry* le = _new LexicalEntry(ID, keyWithoutVowels, fvs, analysis, num_sub_items);
		lex->addStaticEntry(le);
	}
	//std::cout<<"read dictionary"<<std::endl;
	//std::cout<<"size: "<<static_cast<int>(lex->getNEntries())<<std::endl;
	
	stream.close();
	return lex;
}
