// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstddef>
#include <string>
#include "Generic/parse/ParserTrainer/HeadlessParseNode.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/trainers/HeadFinder.h"
#include "Generic/trainers/Production.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/parse/ParserTrainer/ParserTrainerLanguageSpecificFunctions.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
using namespace std;
const size_t HeadlessParseNode::blockSize = HEADLESS_PARSE_NODE_BLOCK_SIZE;
HeadlessParseNode* HeadlessParseNode::freeList = 0;

// make sure to start after reading a '(' 
void HeadlessParseNode::read (UTF8InputStream& stream)
{
	HeadlessParseNode* n;
	UTF8Token token;
	
	stream >> token;
	if (wcscmp (token.chars(), L")") == 0) {
		label = Symbol(L"EMPTY_PARSE");
		return;
	}
	
	label = Symbol(token.chars());
	stream >> token;

	// word pair (base case)
	if (wcscmp (token.chars(), L"(") != 0) {
		children = new HeadlessParseNode(Symbol(token.chars()), _headFinder);
		isPreTerminal = true;
		stream >> token; // ')'
		return;
	} else isPreTerminal = false;

	// read children
	children = new HeadlessParseNode(_headFinder);
	n = children;
	n->read(stream);
	
	stream >> token; // either '(' or ')'

	// read siblings
	while (wcscmp(token.chars(), L"(") == 0) {
		n->next = new HeadlessParseNode(_headFinder);
		n = n->next;
		n->read(stream);
		stream >> token; // either '(' or ')'
	}

	return;

}


wstring HeadlessParseNode::Headify_to_string()
{
	HeadlessParseNode* n;
	HeadlessParseNode* old_n;
	wstring result = L"";

	if (isPreTerminal) {
		result += L" (";
		result += label.to_string();
        result += L' ';
		result += children->label.to_string();
		result += L')';
		return result;
	}

	_headFinder->production.left = label;

	int index = 0;

	// fill in Production right-side array
	n = children;
	while (n != 0) {
		_headFinder->production.right[index] = n->label;
		n = n->next;
		index++;
		if (index >= MAX_RIGHT_SIDE_SYMBOLS)
			throw UnexpectedInputException("HeadlessParseNode::HeadifyToString", 
			"Production right-side array too small");
	}

	_headFinder->production.number_of_right_side_elements = index;

	// find index of head element
	int j = _headFinder->get_head_index();
	int k = 0;

	// set n = head element
	old_n = 0;
	n = children;
	while (k < j) {
		old_n = n;
		n = n->next;
		k++;
	}

	//NOTE: Should this be language specific?
	bool no_children_np = true;
	if(LanguageSpecificFunctions::isNPtypeLabel(label)){
		if(containsNP())
			no_children_np = false;
	}
	// adjust labels
	label = ParserTrainerLanguageSpecificFunctions::
		adjustNPLabelsForTraining(label, n->label, n->isPreTerminal);
	/*
	//This is only true for Arabic.  Dont Check this in!!!!!
	label = ParserTrainerLanguageSpecificFunctions::
		adjustNPLabelsForTraining(label, n->label, no_children_np);
	*/
	// change labels for proper nouns in names
	// NB: this happens AFTER we have found the head element,
	//     so these new labels don't affect head-finding

	label = ParserTrainerLanguageSpecificFunctions::adjustNameLabelForTraining(label);
	// this will only change labels for certain things, but run the check
	// for all nodes, since we're trying to keep language specific stuff elsewhere.
	HeadlessParseNode* node = children;
	while (node != 0) {
		node->label = ParserTrainerLanguageSpecificFunctions::
			adjustNameChildrenLabelForTraining(label, node->label);
		node = node->next;
	}

	result = L" (";
	result += label.to_string();
	if (old_n == 0) {
		result += L" (PRE) (HEAD";
	} else {
		// this separates the pre chain from the rest
		old_n->next = 0;
		result += L" (PRE";
		addModsToString(children, result);
		result += L") (HEAD";
	}
	result += n->Headify_to_string();
	result += L") (POST";
	addModsToString(n->next, result);
	result += L"))";
	// don't forget to reconnect the chain (for deletion later)
	if (old_n != 0) 
		old_n->next = n;
	return result;
	
}

void HeadlessParseNode::addModsToString(HeadlessParseNode* modifier, wstring& result)
{
	if (modifier) {
		result += modifier->Headify_to_string();
		addModsToString(modifier->next, result);
	}
}


void* HeadlessParseNode::operator new(size_t)
{
    HeadlessParseNode* p = freeList;
    if (p) {
        freeList = p->next;
    } else {
        HeadlessParseNode* newBlock = static_cast<HeadlessParseNode*>(::operator new(
            blockSize * sizeof(HeadlessParseNode)));
        for (size_t i = 1; i < (blockSize - 1); i++)
            newBlock[i].next = &newBlock[i + 1];
        newBlock[blockSize - 1].next = 0;
        p = newBlock;
        freeList = &newBlock[1];
    }
    return p;
}

void HeadlessParseNode::operator delete(void* object)
{
    HeadlessParseNode* p = static_cast<HeadlessParseNode*>(object);
    p->next = freeList;
    freeList = p;
}

bool HeadlessParseNode::containsNP(){
	HeadlessParseNode* c = children;
	while(c != 0){
		if(LanguageSpecificFunctions::isNPtypeLabel(c->label)){
			return true;
		}
		c = c->next;
	}
	c = children;
	while(c != 0){
		if(c->containsNP())
			return true;
		c  = c->next;
	}
	return false;
}


