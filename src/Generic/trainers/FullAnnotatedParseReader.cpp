// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/trainers/CorefItem.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/trainers/Production.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "Generic/trainers/FullAnnotatedParseReader.h"
#include <boost/algorithm/string.hpp>
//#include <iostream.h>
using namespace std;

FullAnnotatedParseReader::FullAnnotatedParseReader() : _headFinder(HeadFinder::build()) { 
}

FullAnnotatedParseReader::FullAnnotatedParseReader(const char *file_name) : SexpReader(file_name), _headFinder(HeadFinder::build()) { 
}


void FullAnnotatedParseReader::readAllParses(GrowableArray <Parse *> & results){
	try {
		Parse *singleParseResult;
		while((singleParseResult = readOptionalParse())!= NULL) {
			results.add(singleParseResult);
		}
	}
	 catch(UnexpectedInputException &e) {
		_debugOut << "\n" << "End document: FAILED. ";
		SessionLogger::err("SERIF")<<"\nRead Error!\n"<<e.getMessage()<<"\n";
		readFailed();
	}
}



Parse * FullAnnotatedParseReader::readOptionalParse() {
	Parse *result = NULL;
	SynNode *node = readOptionalNode();
	if(node != NULL) 
		result = _new Parse(NULL, node, 0.0);
	return result;
}


SynNode* FullAnnotatedParseReader::readOptionalNodeWithTokens(int& tok_num){
	
		SynNode *result = _new SynNode(0, 0, Symbol(L""), 0);

		_debugOut.setIndent(_debugOut.getIndent()+1);
	//_debugOut << "\n" << "Begin Node: ";
	startRead();
	try {

		UTF8Token token = getToken(LPAREN | WORD);

		if(token.symValue() != SymbolConstants::leftParen) {
			//word, thus, this is a terminal node
			std::wstring word(token.chars());
			//std::transform(word.begin(), word.end(), word.begin(), towlower);
			result->setTag(Symbol(word.c_str()));
			result->setTokenSpan(tok_num, tok_num);
			tok_num++;
			//_debugOut << "\n" << "End Node: SUCCESS";
			readSucceeded();
			_debugOut.setIndent(_debugOut.getIndent()-1);
			return result;
		}
		
		token = getWord();

		//first, search for '/'
		Symbol tag = Symbol(token.chars());

		result->setTag(tag);
		int start_token = tok_num;
		//now, recursively find children
		SynNode *child = NULL;
		const int MAX_CHILDREN = 2000; //was 250 for arabic name linking
		SynNode* children[MAX_CHILDREN];
		int nChildren = 0;
		while((child = readOptionalNodeWithTokens(tok_num)) != NULL) {
			child->setParent(result);
			if(nChildren+1> MAX_CHILDREN)
				throw InternalInconsistencyException("AnnotatedParseReader::readOptionalNode()", "MAX_CHILDREN exceeded.");
			children[nChildren++] = child;
		}
		if(nChildren != 0)
			result->setChildren(nChildren, 0, children);
		result->setTokenSpan(start_token, tok_num - 1);
		//now we have to find the head index
		_headFinder->production.number_of_right_side_elements = 0;
		_headFinder->production.left = result->getTag();
		int sz = nChildren < 75 ? nChildren :75;	//mf added for too many right side elements 
		for(int i=0; i<sz; i++) {

			_headFinder->production.right[_headFinder->production.number_of_right_side_elements++]
				= children[i]->getTag();
		}
		int head_index = _headFinder->get_head_index(); 
		result->setHeadIndex(head_index);

		getRightParen();
		
		// TODO: flatten all Name sub-trees to match Serif processing?

	} catch(UnexpectedInputException &) {
		//_debugOut << "\n" << "End Node: FAILED";
		readFailed();
		_debugOut.setIndent(_debugOut.getIndent()-1);
		return NULL;
	}
	//_debugOut << "\n" << "End Node: SUCCESS";
	readSucceeded();
	_debugOut.setIndent(_debugOut.getIndent()-1);
	
	return result;
	}
SynNode * FullAnnotatedParseReader::readOptionalNode() {
	int tok_num = 0;
	return readOptionalNodeWithTokens(tok_num);

}

//had to copy these b/c they are private
int FullAnnotatedParseReader::readOptionalID() {
	startRead();
	int result = CorefItem::NO_ID;

	UTF8Token token = getToken(LPAREN | WORD);
	if(token.symValue() == SymbolConstants::leftParen) {
		readFailed();
		return result;
	}

	const wchar_t *word = token.chars();
	// = can be in a token, too
	const wchar_t *loc = wcsstr(word, L"ID=");
	if(loc != NULL) {
		//advance to the equals sign
		loc += 2;
		wchar_t second[256];
		size_t index = loc-word;
		size_t nWord = wcslen(word);
		wcsncpy(second, loc+1, nWord-(index+1));
		second[nWord-(index+1)] = L'\0';
		result = wcstol(second, NULL, 10);
		readSucceeded();
		return result;
	}
	else {
		readFailed();
		return result;
	}
}

Mention *FullAnnotatedParseReader::processAugmentedTag(const wchar_t *augmented_tag, Symbol &tag) {
	const wchar_t *loc1 = wcschr(augmented_tag, L'/');
	
	if(loc1 == NULL) {
		//this is a simple tag, not augmented
		tag = Symbol(augmented_tag);
		return NULL;
	}
	const wchar_t *loc2 = wcschr(loc1+1, L'/');

	if(loc2 == NULL) {
		throw InternalInconsistencyException("AnnotatedParseReader::processAugmentedTag", 
			"improper augmentation format");
	}

	Mention *mentionResult;
	size_t index1 = loc1-augmented_tag;
	size_t index2 = loc2-(loc1+1);
	size_t nAugmentedTag = wcslen(augmented_tag);

	wchar_t first[256], second[256], third[256];
	wcsncpy(first, augmented_tag, index1);
	first[index1] = L'\0';
	wcsncpy(second, loc1+1, index2);
	second[index2] = L'\0';
	wcsncpy(third, loc2+1, nAugmentedTag-(index1+1+index2+1));
	third[nAugmentedTag-(index1+1+index2+1)] = L'\0';
	tag = Symbol(third);
	//now, find out what entity type this node is
	mentionResult = _new Mention();
	mentionResult->setEntityType(Symbol(first));
	// TODO: check on types, and set to OTH if not 
	// a registered type (also warn)

	//now, mention type
	// this string comparison has to be done because 
	// mention types aren't stored as strings. Otherwise 
	// we could do as above.
	if(wcsstr(const_cast<const wchar_t *>(second),L"PRON")!= NULL)
		mentionResult->mentionType = Mention::PRON;
	else if (wcsstr(const_cast<const wchar_t *>(second), L"DESC")!=NULL)
		mentionResult->mentionType = Mention::DESC;
	else if (wcsstr(const_cast<const wchar_t *>(second), L"NAME")!=NULL)
		mentionResult->mentionType = Mention::NAME;
	// HACK: ideally we'd want this to be a list, but we aren't building
	// list structure, so just make it a nothing and ignore it
	else if (wcsstr(const_cast<const wchar_t *>(second), L"LIST")!=NULL)
		mentionResult->mentionType = Mention::NONE;
	
	return mentionResult;
}

