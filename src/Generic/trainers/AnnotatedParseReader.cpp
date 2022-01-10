// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/trainers/AnnotatedParseReader.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/trainers/CorefItem.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/trainers/Production.h"
#include "Generic/parse/LanguageSpecificFunctions.h"

#include <boost/algorithm/string.hpp>
//#include <iostream.h>
using namespace std;

AnnotatedParseReader::AnnotatedParseReader() : _headFinder(HeadFinder::build()) { 
}

AnnotatedParseReader::AnnotatedParseReader(const char *file_name) : SexpReader(file_name) { 
}

void AnnotatedParseReader::readAllDocuments(GrowableArray <CorefDocument *> & results) 
{
	CorefDocument *thisResult;
	while((thisResult = readOptionalDocument()) != NULL)
		results.add(thisResult);
	getEOF();
}

CorefDocument *AnnotatedParseReader::readOptionalDocument() {
	CorefDocument *result = _new CorefDocument();
	static int num = 0;
	num++;
// TODO: What's this for??
//	if(num > 0)
//		_debugOut.setActive(true);
//	else _debugOut.setActive(false);
	_debugOut << "\n" << "Begin Document: ";
	startRead();
	try {
		Parse *singleParseResult;
		getLeftParen();
		result->documentName = getWord().symValue();
		cout << "\nReading document \"" << result->documentName.to_debug_string() << "\"";
		while((singleParseResult = readOptionalParse(result->corefItems))!= NULL) {
			result->parses.add(singleParseResult);
		}
		getRightParen();
	} catch(UnexpectedInputException &) {
		_debugOut << "\n" << "End document: FAILED. ";
		readFailed();
		delete result;
		return NULL;
	}
	_debugOut << "\n" << "End document: SUCCESS";
	readSucceeded();
	return result;
}

Parse * AnnotatedParseReader::readOptionalParse(GrowableArray <CorefItem *> & corefItemResults) {
	Parse *result = NULL;
	SynNode *node = readOptionalNode(corefItemResults);
	if(node != NULL) 
		result = _new Parse(NULL, node, 0.0);
	return result;
}

SynNode * AnnotatedParseReader::readOptionalNodeWithTokens(GrowableArray <CorefItem *> & corefItemResults, 
														   int& tok_num) 
{
	SynNode *result = _new SynNode(0, 0, Symbol(L""), 0);
	//Mention *mentionResult = NULL;
	CorefItem *corefItemResult = NULL;

	//result->setTokenSpan(-1, -1);

	_debugOut.setIndent(_debugOut.getIndent()+1);
	//_debugOut << "\n" << "Begin Node: ";
	startRead();
	int rollbackLength = corefItemResults.length();

	try {

		UTF8Token token = getToken(LPAREN | WORD);

		if(token.symValue() != SymbolConstants::leftParen) {
			//word, thus, this is a terminal node
			std::wstring word(token.chars());
			std::transform(word.begin(), word.end(), word.begin(), towlower);
			result->setTag(Symbol(word.c_str()));
			result->setTokenSpan(tok_num, tok_num);
			tok_num++;
			//_debugOut << "\n" << "End Node: SUCCESS";
			readSucceeded();
			_debugOut.setIndent(_debugOut.getIndent()-1);
			return result;
		}

		
		token = getWord();

		const wchar_t *augmented_tag = token.chars();
		//first, search for '/'
		Symbol tag;
		corefItemResult = NULL;
		Mention *mentionResult = NULL;
		mentionResult = processAugmentedTag(augmented_tag, tag);
		int id = readOptionalID();
		//if this node has either a corefID or a mention, give it a CorefItem
		if((id != CorefItem::NO_ID) || (mentionResult != NULL)) {
			corefItemResult = _new CorefItem();
			if(mentionResult != NULL) {
				mentionResult->node = result;
				corefItemResult->mention = mentionResult;
			}
			if(id != CorefItem::NO_ID) {
				corefItemResult->setID(id);
				if((id = readOptionalID()) != CorefItem::NO_ID)
					corefItemResult->setID(id);
			}
			corefItemResult->node = result;
			corefItemResults.add(corefItemResult);
			result->setMentionIndex(corefItemResults.length()-1);
		}
		result->setTag(tag);
		int start_token = tok_num;
		//now, recursively find children
		SynNode *child = NULL;
		const int MAX_CHILDREN = 250; //was 100, increased for arabic name linking and chinese
		SynNode* children[MAX_CHILDREN];
		int nChildren = 0;
		while((child = readOptionalNodeWithTokens(corefItemResults, tok_num)) != NULL) {
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
		LanguageSpecificFunctions::modifyParse(result, corefItemResult, children, nChildren, head_index);

	} catch(UnexpectedInputException &) {
		//_debugOut << "\n" << "End Node: FAILED";
		readFailed();
		_debugOut.setIndent(_debugOut.getIndent()-1);
		while(corefItemResults.length() != rollbackLength)
			delete corefItemResults.removeLast();
		//delete result;
		return NULL;
	}
	//_debugOut << "\n" << "End Node: SUCCESS";
	readSucceeded();
	_debugOut.setIndent(_debugOut.getIndent()-1);
	return result;
}

SynNode * AnnotatedParseReader::readOptionalNode(GrowableArray <CorefItem *> & corefItemResults) {
	int tok_num = 0;
	return readOptionalNodeWithTokens(corefItemResults, tok_num);

}

Mention *AnnotatedParseReader::processAugmentedTag(const wchar_t *augmented_tag, Symbol &tag) {
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
	
	size_t index1 = loc1-augmented_tag;
	size_t index2 = loc2-(loc1+1);
	size_t nAugmentedTag = wcslen(augmented_tag);

	wchar_t ent_type_str[256], ment_type_str[256], tag_str[256];
	wcsncpy(ent_type_str, augmented_tag, index1);
	ent_type_str[index1] = L'\0';
	wcsncpy(ment_type_str, loc1+1, index2);
	ment_type_str[index2] = L'\0';
	wcsncpy(tag_str, loc2+1, nAugmentedTag-(index1+1+index2+1));
	tag_str[nAugmentedTag-(index1+1+index2+1)] = L'\0';
	tag = Symbol(tag_str);

	// look for a subtype in the entity type string
	EntitySubtype subtype = EntitySubtype::getUndetType();
	wchar_t *dot = wcschr(ent_type_str, L'.');
	if (dot != NULL) {
		try {
			subtype = EntitySubtype(Symbol(ent_type_str));
		} 
		catch(UnrecoverableException &e) {
			cerr << "\n" << e.getSource() << ": " << e.getMessage() << "\n";
			subtype = EntitySubtype::getUndetType();
		}
		size_t dot_index = dot - ent_type_str;
		ent_type_str[dot_index] = L'\0';
	}

	Mention *mentionResult = _new Mention();
	//now, find out what entity type this node is
	mentionResult->setEntityType(EntityType(Symbol(ent_type_str)));
	mentionResult->setEntitySubtype(subtype);
	// TODO: check on types, and set to OTH if not 
	// a registered type (also warn)

	//now, mention type
	// this string comparison has to be done because 
	// mention types aren't stored as strings. Otherwise 
	// we could do as above.
	if((wcsstr(const_cast<const wchar_t *>(ment_type_str), L"PRON") != NULL) || (wcsstr(const_cast<const wchar_t *>(ment_type_str), L"PRO") != NULL))
		mentionResult->mentionType = Mention::PRON;
	else if (wcsstr(const_cast<const wchar_t *>(ment_type_str), L"DESC") != NULL)
		mentionResult->mentionType = Mention::DESC;
	else if (wcsstr(const_cast<const wchar_t *>(ment_type_str), L"NAME") != NULL)
		mentionResult->mentionType = Mention::NAME;
	// HACK: ideally we'd want this to be a list, but we aren't building
	// list structure, so just make it a nothing and ignore it
	else if (wcsstr(const_cast<const wchar_t *>(ment_type_str), L"LIST") != NULL)
		mentionResult->mentionType = Mention::NONE;
	
	return mentionResult;
}



// tries to read coref information, or returns CorefItem::NO_ID if there is no coref information
int AnnotatedParseReader::readOptionalID() {
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
	if (loc != NULL) {	
		// advance past the equals sign and open quote
		loc += 3;
		if (*loc == '"')
			loc++;
		
		// copy just the digits into a new string
		wchar_t second[256];
		size_t count = wcslen(loc);
		wcsncpy(second, loc, count);
		if (count > 0 && second[count-1] == '"')
			second[count-1] = L'\0';
		else
			second[count] = L'\0';

		// convert string to int
		result = wcstol(second, NULL, 10);
		readSucceeded();
		return result;
	}
	else {
		readFailed();
		return result;
	}
}

