// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <cstddef>
#include <string>
#include "Generic/parse/ParseNode.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/parse/ParserTags.h"

#include "Generic/parse/LanguageSpecificFunctions.h"


using namespace std;


const size_t ParseNode::blockSize = PARSE_NODE_BLOCK_SIZE;
ParseNode* ParseNode::freeList = 0;

string ParseNode::toDebugString()
{	
if (headNode == 0){
        return string(label.to_debug_string());
    } else {
        string result = "(";
        result += label.to_debug_string();
        addPremodsToDebugString(premods, result);
        result += ' ';
        result += headNode->toDebugString();
        addPostmodsToDebugString(postmods, result);
        result += ')';
        return result;
    }
}

void ParseNode::addPremodsToDebugString(ParseNode* premod, string& str)
{
    if (premod) {
       addPremodsToDebugString(premod->next, str);
       str += ' ';
       str += premod->toDebugString();
    }
}

void ParseNode::addPostmodsToDebugString(ParseNode* postmod, string& str)
{
    if (postmod) {
       str += ' ';
       str += postmod->toDebugString();
       addPostmodsToDebugString(postmod->next, str);
    }
}
wstring ParseNode::toWString(bool printHeadInfo)
{
    if (headNode == 0) {
        return wstring(label.to_string());
    } else {
        wstring result = L"(";
        result += label.to_string();
        addPremodsToString(premods, result, printHeadInfo);
        result += L' ';
		if (printHeadInfo && headNode->headNode != 0)
			result += L"HEAD_NEXT ";
        result += headNode->toWString(printHeadInfo);
        addPostmodsToString(postmods, result, printHeadInfo);
        result += L')';
        return result;
    }
}

void ParseNode::addPremodsToString(ParseNode* premod, wstring& str, bool printHeadInfo)
{
    if (premod) {
       addPremodsToString(premod->next, str, printHeadInfo);
       str += ' ';
       str += premod->toWString(printHeadInfo);
    }
}

void ParseNode::addPostmodsToString(ParseNode* postmod, wstring& str, bool printHeadInfo)
{
    if (postmod) {
       str += ' ';
       str += postmod->toWString(printHeadInfo);
       addPostmodsToString(postmod->next, str, printHeadInfo);
    }
}
wstring ParseNode::toNPChunkString(Symbol& chunk_tag)
{	
	//this should never happen
    if (headNode == 0) {
        wstring result = L"";
		return result;
    } 
	else if(headNode->headNode == 0){ //this is a pre-terminal
		wstring result = L"";
		if(label == ParserTags::TOPTAG){
			return result;
		}
		Symbol pos =label;
		Symbol word = headNode->label;
		result += L" ( ";
		result += word.to_string();
		result += L' ';
		result += pos.to_string();
		result += L' ';
		if(LanguageSpecificFunctions::isNoun(pos) && chunk_tag == Symbol(L"NONE-ST")){ 
			//special case (NP (NN XXX) (NPA XXX)) add an NPChunk over the noun

			result += L"NP-ST"; 
		}
		else{
			result += chunk_tag.to_string();
		}
		result += L" )";
		if(chunk_tag == Symbol(L"NP-ST")){
			chunk_tag = Symbol(L"NP-CO");
		}
		return result;
	}
	else {
		bool is_npa = label == Symbol(L"NPA");
		bool set_npa = false;
		if(is_npa){
			if(chunk_tag == Symbol(L"NONE-ST")){
				chunk_tag = Symbol(L"NP-ST");
				set_npa = true;
			}
			else{
				SessionLogger::warn("SERIF")<<"ERROR Recursive NPA!"<<std::endl
					<<toDebugString().c_str()<<std::endl;
			}
		}
        wstring result = L"";
        addPremodsToNPChunkString(premods, result, chunk_tag);
        result += headNode->toNPChunkString(chunk_tag);
        addPostmodsToNPChunkString(postmods, result, chunk_tag);
		if(set_npa){
			chunk_tag = Symbol(L"NONE-ST");
		}
        return result;
    }
}

void ParseNode::addPremodsToNPChunkString(ParseNode* premod, wstring& str, Symbol& chunk_tag)
{
    if (premod) {
       addPremodsToNPChunkString(premod->next, str, chunk_tag);
       str += premod->toNPChunkString(chunk_tag);
    }
}

void ParseNode::addPostmodsToNPChunkString(ParseNode* postmod, wstring& str, Symbol& chunk_tag)
{
    if (postmod) {
       str += postmod->toNPChunkString(chunk_tag);
       addPostmodsToNPChunkString(postmod->next, str, chunk_tag);
    }
}
/*
//Includes Arabic Specific calls- use to make nps that are headed by a definite noun,
//definite


void ParseNode::makeNPSDefinite(){
	if(headNode == 0){
		return;
	}
	//save time, skip preterms
	if(headNode->headNode == 0){
		return;
	} 
	if(label == STags::NP){
		ParseNode* preterm = getHeadPreterm();
		if(LanguageSpecificFunctions::isDefiniteNounTag(preterm->label)){
			label = STags::DEF_NP;
		}
	}
	else if(label == STags::NPA){
		ParseNode* preterm = getHeadPreterm();
		if(LanguageSpecificFunctions::isDefiniteNounTag(preterm->label)){
			label = STags::DEF_NPA;
		}
	}
	//order doesn't matter here since we're always getting the head word
	ParseNode* pm = premods;
	while(pm){
		pm->makeNPSDefinite();
		pm = pm->next;
	}
	headNode->makeNPSDefinite();
	pm = postmods;
	while(pm){
		pm->makeNPSDefinite();
		pm = pm->next;
	}
}

*/


ParseNode* ParseNode::getHeadPreterm(){
	if(headNode->headNode == 0){
		return this;
	}
	return headNode->getHeadPreterm();
}






void* ParseNode::operator new(size_t)
{
    ParseNode* p = freeList;
    if (p) {
        freeList = p->next;
    } else {
		//cerr << "allocating new ParseNode block" << endl;
        ParseNode* newBlock = static_cast<ParseNode*>(::operator new(
            blockSize * sizeof(ParseNode)));
        for (size_t i = 1; i < (blockSize - 1); i++)
            newBlock[i].next = &newBlock[i + 1];
        newBlock[blockSize - 1].next = 0;
        p = newBlock;
        freeList = &newBlock[1];
    }
    return p;
}

void ParseNode::operator delete(void* object)
{
    ParseNode* p = static_cast<ParseNode*>(object);
    p->next = freeList;
    freeList = p;
}

void ParseNode::read_from_file(UTF8InputStream& stream, bool IFW) 
{

	ParseNode* n;
	UTF8Token token;

	stream >> token;

	label = Symbol(token.chars());

	stream >> token;

	// word pair (base case)
	if (token.symValue() != ParserTags::leftParen) { 
		headNode = _new ParseNode(Symbol(token.chars()));
		headWordNode = this;
		isFirstWord = IFW;
		stream >> token;
		return;
	}

    stream >> token; // PRE
	stream >> token; 

	// read in premodifiers into isolated linked chain
	if (token.symValue() != ParserTags::rightParen) {	//mf- changed back to right
		ParseNode* temp_chain_beginning = _new ParseNode();
		temp_chain_beginning->read_from_file(stream, IFW);
		ParseNode* temp_node = temp_chain_beginning;
		stream >> token;
		IFW = false;
		while (token.symValue() != ParserTags::rightParen) {
			temp_node->next = _new ParseNode();
			temp_node = temp_node->next;
			temp_node->read_from_file(stream, IFW);
			stream >> token;
		}
		// reverse the chain and connect its beginning to ->premods
		premods_reverse(temp_chain_beginning, temp_chain_beginning->next);
		premods = temp_node;
		temp_chain_beginning->next = 0;
	}
	
	stream >> token; // (
	stream >> token; // HEAD
	stream >> token; // (
	headNode = _new ParseNode();
	headNode->read_from_file(stream, IFW);
	if (headNode->label != ParserTags::TOPTAG)
		IFW = false;
	stream >> token; // )
	
	stream >> token; // (
	stream >> token; // POST
	stream >> token;

	if (token.symValue() != ParserTags::rightParen) {
		postmods = _new ParseNode();
		n = postmods;
		n->read_from_file(stream, IFW);
		stream >> token;
	}
	while (token.symValue() != ParserTags::rightParen) {
		n->next = _new ParseNode();
		n = n->next;
		n->read_from_file(stream, IFW);
		stream >> token;
	}

	stream >> token; // )

	// so we can easily access the actual head word of any node
	headWordNode = headNode->headWordNode;
	return;
}


void ParseNode::premods_reverse(ParseNode* first_node, ParseNode* second_node)
{
	if (second_node == 0) {
		return;
	} else {
		premods_reverse (second_node, second_node->next);
		second_node->next = first_node;
	}
}

wstring ParseNode::to_headified_wstring()
{

	wstring result = L"";

	if (headNode->headNode == 0) {
		result += L" (";
		result += label.to_string();
        result += L' ';
		result += headNode->label.to_string();
		result += L')';
		return result;
	}
	
	result = L" (";
	result += label.to_string();
	result += L" (PRE";
	addHeadifiedPremodsToString(premods, result);
	result += L") (HEAD";
	result += headNode->to_headified_wstring();
	result += L") (POST";
	addHeadifiedPostmodsToString(postmods, result);
	result += L"))";
	return result;
	
}


void ParseNode::addHeadifiedPremodsToString(ParseNode* premod, wstring& str)
{
    if (premod) {
       addHeadifiedPremodsToString(premod->next, str);
       str += ' ';
       str += premod->to_headified_wstring();
    }
}

void ParseNode::addHeadifiedPostmodsToString(ParseNode* postmod, wstring& str)
{
    if (postmod) {
       str += ' ';
       str += postmod->to_headified_wstring();
       addHeadifiedPostmodsToString(postmod->next, str);
    }
}

//added for error checking in arabic parser
wstring ParseNode::readLeaves(){
    if (headNode == 0) {
        return wstring(label.to_string());
	}
	else{
		wstring result;
		getPremodLeaves(premods, result);
		result += headNode->readLeaves();
		getPostmodLeaves(postmods, result);
		return result;
	}
}
void ParseNode::getPremodLeaves(ParseNode* premod, wstring& str)
{
    if (premod) {
       getPremodLeaves(premod->next, str);
       str += premod->readLeaves();
    }
}

void ParseNode::getPostmodLeaves(ParseNode* postmod, wstring& str)
{
    if (postmod) {
      str += postmod->readLeaves();
      getPostmodLeaves(postmod->next, str);
    }
}

//write just the Terminals (S (W ...) (W ...) (W ...) )


wstring ParseNode::writeTokens(){
    if (headNode->headNode == 0) {
		wstring result = L"";
		result+=L" ( W ";	
		//result+=wstring(label.to_string());
		//result+=L" ";
		result+=wstring(headNode->label.to_string());
		result+=L" )";
		return result;
	}
	else{
		wstring result;
		getPremodTokens(premods, result);
		result += headNode->writeTokens();
		getPostmodTokens(postmods, result);
		return result;
	}
}
void ParseNode::getPremodTokens(ParseNode* premod, wstring& str)
{
    if (premod) {
       getPremodTokens(premod->next, str);
       str += premod->writeTokens();
    }
}

void ParseNode::getPostmodTokens(ParseNode* postmod, wstring& str)
{
    if (postmod) {
      str += postmod->writeTokens();
      getPostmodTokens(postmod->next, str);
    }
}

