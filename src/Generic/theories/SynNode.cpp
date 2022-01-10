// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/StateLoader.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/state/XMLTheoryElement.h"
#include "Generic/state/XMLSerializedDocTheory.h"
#include "Generic/state/XMLStrings.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/MentionSet.h"

#include <stdio.h>

using namespace std;

SynNode::SynNode(int ID, SynNode *parent, const Symbol &tag, int n_children)
	: _ID(ID), _parent(parent), _tag(tag), _n_children(n_children),
	  _head_index(0), _mention_index(-1)
{
	if (n_children == 0)
		_children = 0;
	else
		_children = _new SynNode*[n_children];
}

SynNode::SynNode()
: _ID(-1), _parent(0), _tag(Symbol()), _n_children(0),
	  _head_index(-1), _mention_index(-1)
{ }

SynNode::SynNode(const SynNode &other, SynNode *parent)
: _ID(other._ID), _tag(other._tag),
_parent(parent), _n_children(other._n_children),
_head_index(other._head_index), _mention_index(other._mention_index),
_start_token(other._start_token), _end_token(other._end_token)
{
	if (_n_children > 0) {
		_children = _new SynNode*[_n_children];
		for (int i = 0; i < _n_children; i++)
			_children[i] = _new SynNode(*(other._children[i]), this);
	} else
		_children = NULL;
}

void SynNode::setChild(int i, SynNode *child) {
	if ((unsigned) i < (unsigned) _n_children)
		_children[i] = child;
	else
		throw InternalInconsistencyException::arrayIndexException(
			"SynNode::setChild()", _n_children, i);
}

SynNode::~SynNode() {
	for (int i = 0; i < _n_children; i++)
		delete _children[i];
	delete[] _children;
}

void SynNode::setChildren(int n, int head_index, SynNode *children[]) {
	delete[] _children;
	_children = _new SynNode*[n];
	_head_index = head_index;
	_n_children = n;
	for(int i=0; i<n; i++) {
		_children[i] = children[i];
		_children[i]->_parent = this;
	}
}

const SynNode *SynNode::getChild(int i) const {
	if ((unsigned) i < (unsigned) _n_children)
		return _children[i];
	else
		throw InternalInconsistencyException::arrayIndexException(
			"SynNode::getChild()", _n_children, i);
}


Symbol SynNode::getSingleWord() const {
	if (_n_children == 0)
		return _tag;
	else if (_n_children == 1)
		return _children[0]->getSingleWord();
	else
		return Symbol(); 
}

const Symbol &SynNode::getHeadWord() const {
	if (_n_children == 0)
		return _tag;
	else
		return _children[_head_index]->getHeadWord();
}

const SynNode *SynNode::getHighestHead() const {
	if (_parent == 0)
		return this;
	else if (_parent->getHead() != this)
		return this;
	else
		return _parent->getHighestHead();
}

const SynNode *SynNode::getHeadPreterm() const {
	if (isPreterminal())
		return this;
	if (isTerminal())
		return _parent;
	else
		return _children[_head_index]->getHeadPreterm();
}

std::vector<Symbol> SynNode::getTerminalSymbols() const {
	std::vector<Symbol> results;
	if (isTerminal()) {
		results.push_back(_tag);
	}
	else {
		for (int i = 0; i < _n_children; i++) {
			std::vector<Symbol> childWords = _children[i]->getTerminalSymbols();
			results.insert(results.end(), childWords.begin(), childWords.end());
		}
	}
	return results;
}

int SynNode::getTerminalSymbols(Symbol results[], int max_results) const {
	if (max_results == 0) {
		return 0;
	}
	else if (isTerminal()) {
		results[0] = _tag;
		return 1;
	}
	else {
		int n = 0; // pos in results array
		for (int i = 0; i < _n_children; i++) {
			n +=
			  _children[i]->getTerminalSymbols(results + n, max_results - n);
		}
		return n;
	}
}

int SynNode::getPOSSymbols(Symbol results[], int max_results) const {
	if (max_results == 0) {
		return 0;
	}
	else if (isPreterminal()) {
		results[0] = _tag;
		return 1;
	}
	else {
		int n = 0; // pos in results array
		for (int i = 0; i < _n_children; i++) {
			n +=
			  _children[i]->getPOSSymbols(results + n, max_results - n);
		}
		return n;
	}
}


const SynNode *SynNode::getFirstTerminal() const {
	const SynNode *node=this;
	while (node->_n_children)
		node = node->_children[0];
	return node;
}

const SynNode *SynNode::getLastTerminal() const {
	const SynNode *node=this;
	while (node->_n_children)
		node = node->_children[node->_n_children-1];
	return node;
}

const SynNode *SynNode::getNthTerminal(int n) const {
	const SynNode *currentNode = getFirstTerminal();
	int i;
	for (i = 0; i < n; i++) {
		if (!currentNode) 
			return NULL;
		currentNode = currentNode->getNextTerminal();	
	}
	return currentNode;
}

const SynNode *SynNode::getCoveringNodeFromTokenSpan(int starttoken, int endtoken) const {
	for(int i=0; i< getNChildren(); i++){
		if((_children[i]->getStartToken()<=starttoken) && 
			(_children[i]->getEndToken()>=endtoken)){
				const SynNode* retnode = _children[i]->getCoveringNodeFromTokenSpan(starttoken, endtoken);
				return retnode;
			}
	}
	return this;
}
const SynNode *SynNode::getNodeByTokenSpan(int starttoken, int endtoken) const {
	//start from the bottom, its faster
	if(starttoken == endtoken){
		const SynNode* thisnode = getNthTerminal(starttoken);
		while((thisnode->getParent() != 0) &&
			(thisnode->getParent()->getStartToken() == starttoken) &&
			(thisnode->getParent()->getEndToken() == endtoken))
		{
				thisnode = thisnode->getParent();
		}
		return thisnode;
	}
	else{
		const SynNode* thisnode = getCoveringNodeFromTokenSpan(starttoken, endtoken);
		if(!(thisnode->getStartToken() == starttoken) && (thisnode->getEndToken() ==endtoken)){
			return 0;
		}
		while((thisnode->getParent()!= 0)&&(thisnode->getParent()->getStartToken() == starttoken) &&
			(thisnode->getParent()->getEndToken() == endtoken))
		{
			thisnode = thisnode->getParent();
		}
		return thisnode;
	}

}
bool SynNode::isAncestorOf(const SynNode* node) const{
	if((getStartToken() <= node->getStartToken()) && (getEndToken() >= node->getEndToken())){
		return true;
	}
	return false;
}
int SynNode::getAncestorDistance(const SynNode* ancestor) const{
	if(!ancestor->isAncestorOf(this)){
		return -1;
	}
	int dist =0;
	const SynNode* node = this;
	while(node != ancestor){
		node = node->getParent();
		dist++;
		if(node == 0){
			return -1;
		}
	}
	return dist;

}

void SynNode::getDescendantMentionIndices(std::vector<int>& mention_indices) const {
	if (_mention_index != -1) {
		mention_indices.push_back(_mention_index);
	}
	for (int j = 0; j < _n_children; j++) {
		_children[j]->getDescendantMentionIndices(mention_indices);
	}	
	return;
}

bool SynNode::hasNextSibling() const {
	if (_parent == 0) {
		return false;
	} else {
		int index = 0;
		while (_parent->_children[index] != this) {
			index++;
		}
		return index < (_parent->_n_children - 1);
	}
}

const SynNode* SynNode::getNextSibling() const {
	if (_parent == 0) {
		return 0;
	} else {
		int index = 0;
		while (_parent->_children[index] != this) {
			index++;
		}
		if (index == (_parent->_n_children - 1)) {
			return 0;
		} else {
			return _parent->_children[index+1];
		}
	}
	
}

const SynNode *SynNode::getNextTerminal() const {
	const SynNode* node = this;
	// Walk up the syntax tree until we're not the last child of our parent.
	while ((node->_parent) && ((node->_parent->_children[node->_parent->_n_children-1]==node)))
		node = node->_parent;
	// If we reached the root, then give up.
	if (!node->_parent)
		return 0;
	// Find our next sibling (which is guaranteed to exist)
	SynNode * const *sibling = node->_parent->_children;
	while (*sibling != node) ++sibling;
	++sibling;
	// Now walk down the tree to get the first terminal.
	return (*sibling)->getFirstTerminal();
}

const SynNode *SynNode::getPrevTerminal() const {
	if (_parent == 0) {
		return 0;
	}
	else {
		if (_parent->_children[0] == this) {
			return _parent->getPrevTerminal();
		}
		else {
			int i = _parent->_n_children - 1;
			while (_parent->_children[i--] != this);
			return _parent->_children[i]->getLastTerminal();
		}
	}
}


bool SynNode::isPreterminal() const {
	return _n_children == 1 &&
	       _children[0]->isTerminal();
}


string SynNode::toDebugString(int indent) const {
	string result = "(";
	result += _tag.to_debug_string();
	result += "  <<";
	char buffer[11];
	_snprintf(buffer, 10, "%d ", _ID);
	result += buffer;
	_snprintf(buffer, 10, "%d:", _start_token);
	result += buffer;
	_snprintf(buffer, 10, "%d", _end_token);
	result += buffer;
	if (hasMention()) {
		result += " -- Mention ";
		_snprintf(buffer, 10, "%d", _mention_index);
		result += buffer;
	}
	result += ">>  ";
	indent += 2;
	bool indicate_head_in_dump_string = false;
	for (int j = 0; j < _n_children; j++) {	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_debug_string();
		}
		else {
			result += "\n";
			for (int i = 0; i < indent; i++) {
				if (j == _head_index && i == indent-1 && indicate_head_in_dump_string)
					result += "*";
				else result += " ";
			}
			result += _children[j]->toDebugString(indent);
		}
	}

	result += ")";

	return result;
}

wstring SynNode::toString(int indent) const {
	wstring result = L"(";
	result += _tag.to_string();
	result += L"  <<";
	wchar_t buffer[11];
	_snwprintf(buffer, 10, L"%d ", _ID);
	result += buffer;
	_snwprintf(buffer, 10, L"%d:", _start_token);
	result += buffer;
	_snwprintf(buffer, 10, L"%d", _end_token);
	result += buffer;
	if (hasMention()) {
		result += L" -- Mention ";
		_snwprintf(buffer, 10, L"%d", _mention_index);
		result += buffer;
	}
	result += L">>  ";
	indent += 2;
	
	for (int j = 0; j < _n_children; j++) {
		result += L" ";	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
		}
		else {
			result += L"\n";
			for (int i = 0; i < indent; i++)
				result += L" ";
			result += _children[j]->toString(indent);
		}
	}

	result += L")";

	return result;
}

wstring SynNode::toFlatString() const {

	wstring result = L"(";
	result += _tag.to_string();
	for (int j = 0; j < _n_children; j++) {
		result += L" ";	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
		}
		else {
			result += _children[j]->toFlatString();
		}
	}

	result += L")";

	return result;
}

wstring SynNode::toTextString() const {
	wstring result = L"";
	for (int j = 0; j < _n_children; j++) {
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
			result += L" ";	
		}
		else {
			result += _children[j]->toTextString();
		}
		/* //one would THINK this would be a better alternative yet apparently many ppl count on trailing spaces...
		result += ( _children[j]->isTerminal() ) ? _children[j]->_tag.to_string() : _children[j]->toTextString();
		if ( j != _n_children-1 ) result += L" ";
		*/
	}
	return result;
}

wstring SynNode::toCasedTextString(const TokenSequence* ts) const {
	wstring result = L"";
	for (int j = 0; j < _n_children; j++) {
		if (_children[j]->isTerminal()) {
			if (_children[j]->_start_token != _children[j]->_end_token) { throw; }

			result += ts->getToken(_children[j]->_start_token)->getSymbol().to_string();
		}
		else {
			result += _children[j]->toCasedTextString(ts);
		}
		if ( j != _n_children-1 ) result += L" ";
	}
	return result;
}

wstring SynNode::toPOSString(const TokenSequence *ts) const {
	wstring result = L"";
	wchar_t numa[100];
	wchar_t* num = numa;
	for (int j = 0; j < _n_children; j++) {
		if (_children[j]->isPreterminal()) {
			result += L"( ";
			result += _children[j]->_children[0]->_tag.to_string();
			result += L" ";
			result += _children[j]->_tag.to_string(); 
			result += L" ";	
			int tokoff = _children[j]->_children[0]->getStartToken();
#ifdef _WIN32

			num = _itow(ts->getToken(tokoff)->getStartEDTOffset().value(),num , 10);

#else
			swprintf (num, sizeof (numa)/sizeof(numa[0]), L"%d",
				  ts->getToken(tokoff)->getStartEDTOffset().value());
#endif
			result += num;
			result += L" ";
#ifdef _WIN32
			num = _itow(ts->getToken(tokoff)->getEndEDTOffset().value(),num , 10);

#else
			swprintf (num, sizeof (numa)/sizeof(numa[0]), L"%d",
				  ts->getToken(tokoff)->getEndEDTOffset().value());
#endif

			result += num;
			result += L" ) ";
		}
		else {
			result += _children[j]->toPOSString(ts);
		}
	}
	return result;
}

wstring SynNode::toEnamexString(MentionSet *mentionSet, Symbol headType) const {
	wstring result = L"";
	bool enamex = false;
	if (hasMention()) {
		const Mention *ment = mentionSet->getMention(getMentionIndex());
		if (ment->getEntityType().isRecognized() &&
			(ment->getEntityType().matchesFAC() ||
			ment->getEntityType().matchesLOC() ||
			ment->getEntityType().matchesORG() ||
			ment->getEntityType().matchesPER() ||
			ment->getEntityType().matchesGPE()))
		{
			if (headType.is_null() &&
				(ment->getMentionType() == Mention::NAME || 
				ment->getMentionType() == Mention::NONE)
				&& _tag == Symbol(L"NPP"))
			{
				enamex = true;
				result += L"<ENAMEX TYPE=\"";
				result += ment->getEntityType().getName().to_string();
				result += L"\">";
			} else if (ment->getMentionType() == Mention::DESC) {
				headType = ment->getEntityType().getName();
			}
		}
	}
	if (isTerminal()) {
		if (!headType.is_null()) {
			result += L"<ENAMEX TYPE=\"";
			result += headType.to_string();
			result += L"_DESC\">";
			result += _tag.to_string(); 
			result += L"</ENAMEX> ";
		} else {
			result += _tag.to_string();
			result += L" ";
		}
	} else {
		for (int j = 0; j < _n_children; j++) {
			if (!headType.is_null() && j == getHeadIndex())
				result += _children[j]->toEnamexString(mentionSet, headType);
			else result += _children[j]->toEnamexString(mentionSet, Symbol());
		}
	}
	if (enamex) result += L"</ENAMEX> ";
	return result;
}

string SynNode::toDebugTextString() const {
	string result = "";
	for (int j = 0; j < _n_children; j++) {
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_debug_string(); 
			result += " ";	
		}
		else {
			result += _children[j]->toDebugTextString();
		}
	}
	return result;
}


std::wstring SynNode::toAugmentedParseString(TokenSequence *tokenSequence,
											 MentionSet *mentionSet, 
											 int indent) const 
{
	std::wstring result = L"";
	if (isTerminal()) {
		result += L" ";		
		result += tokenSequence->getToken(getStartToken())->getSymbol().to_string();
		return result;
	}		
	result += L"\n";
	for (int i = 0; i < indent; i++) {
		result += L" ";
	}
	result += L"(";
	result += getTag().to_string();
    if (hasMention()) {
		const Mention *m = mentionSet->getMentionByNode(this);
		if ((m->getEntityType().isRecognized() && (m->getMentionType() == Mention::NAME ||
												   m->getMentionType() == Mention::DESC ||
												   m->getMentionType() == Mention::PART)) ||
			m->getMentionType() == Mention::PRON)
		{
			if (m->getMentionType() != Mention::PRON) {
				result += L"/";
				result += m->getEntityType().getName().to_string();
				result += L"/";
				result += m->getEntitySubtype().getName().to_string();				
				result += L"/";
				if (m->getMentionType() == Mention::NAME)
					result += L"name/";
				else if	(m->getMentionType() == Mention::DESC)
					result += L"desc/";
				else if (m->getMentionType() == Mention::PART)
					result += L"desc/";
			} else {
				result += L"/UNDET/UNDET/pron/";
			}			
			wchar_t buffer[11];
			_snwprintf(buffer, 10, L"%d ", m->getUID().toInt());	
			result += buffer;	
		}
	} 
	for (int j = 0; j < getNChildren(); j++) {
		result += getChild(j)->toAugmentedParseString(tokenSequence, mentionSet, indent + 2);
	}
	result += L")";
	return result;
}

void SynNode::getAllNonterminalNodesWithHeads( int * heads, int * tokenStarts, int * tokenEnds, Symbol * tags, int & n_nodes, int max_nodes ) const {
	
	if( n_nodes == max_nodes ) return;
	
	heads[n_nodes] = getHeadIndex();
	tokenStarts[n_nodes] = getStartToken();
	tokenEnds[n_nodes] = getEndToken();
	tags[n_nodes++] = getTag();
	
	for (int j = 0; j < getNChildren(); j++) {
		if (!getChild(j)->isTerminal()) {
			getChild(j)->getAllNonterminalNodesWithHeads( heads, tokenStarts, tokenEnds, tags, n_nodes, max_nodes );
		}
	}
}

void SynNode::getAllNonterminalNodes( int * tokenStarts, int * tokenEnds, Symbol * tags, int & n_nodes, int max_nodes ) const {
	
	if( n_nodes == max_nodes ) return;
	
	tokenStarts[n_nodes] = getStartToken();
	tokenEnds[n_nodes] = getEndToken();
	tags[n_nodes++] = getTag();
	
	for (int j = 0; j < getNChildren(); j++) {
		if (!getChild(j)->isTerminal()) {
			getChild(j)->getAllNonterminalNodes( tokenStarts, tokenEnds, tags, n_nodes, max_nodes );
		}
	}
}

void SynNode::getAllNonterminalNodes(std::vector<const SynNode*>& nodes) const {
	if (this->isTerminal()) { return; }
	nodes.push_back(this);
	for (int j = 0; j < this->getNChildren(); j++) {
		this->getChild(j)->getAllNonterminalNodes(nodes);
	}
}

void SynNode::getAllTerminalNodes(std::vector<const SynNode*>& nodes) const {
	if (this->isTerminal()) { 
		nodes.push_back(this); 
	} else {
		for (int j = 0; j < this->getNChildren(); j++) {
			this->getChild(j)->getAllTerminalNodes(nodes);
		}
	}
}

wstring SynNode::toPrettyParse(int indent) const {
	wstring result = L"(";
	result += _tag.to_string();
	indent += 2;
	
	for (int j = 0; j < _n_children; j++) {
		result += L" ";	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
		}
		else {
			result += L"\n";
			for (int i = 0; i < indent; i++)
				result += L" ";
			result += _children[j]->toPrettyParse(indent);
		}
	}

	result += L")";

	return result;
}

wstring SynNode::toPrettyParseWithMentions(const Mention *ment, Symbol label, int indent) const {
	wstring result = L"(";
	result += _tag.to_string();
	const SynNode *mentNode = ment->getNode();
	if (this == mentNode || (_start_token == mentNode->getStartToken() && _end_token == mentNode->getEndToken())) {
		result += L"-"; 
		result += label.to_string();
	}

	indent += 2;

	for (int j = 0; j < _n_children; j++) {
		result += L" ";	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
		}
		else {
			result += L"\n";
			int current_indent = indent;
			if (j == _head_index) current_indent--;
			for (int i = 0; i < current_indent; i++)
				result += L" ";
			if (j == _head_index) result += L"*";
			result += _children[j]->toPrettyParseWithMentions(ment, label, indent);
		}
	}

	result += L")";

	return result;
}

wstring SynNode::toPrettyParseWithMentions(const Mention *ment1, Symbol label1, const Mention *ment2, Symbol label2, int indent) const {
	wstring result = L"(";
	result += _tag.to_string();
	const SynNode *ment1Node = ment1->getNode();
	const SynNode *ment2Node = ment2->getNode();
	if (this == ment1Node || (_start_token == ment1Node->getStartToken() && _end_token == ment1Node->getEndToken())) {
		result += L"-"; 
		result += label1.to_string();
	}
	if (this == ment2Node || (_start_token == ment2Node->getStartToken() && _end_token == ment2Node->getEndToken())) {
		result += L"-"; 
		result += label2.to_string();
	}

	indent += 2;

	for (int j = 0; j < _n_children; j++) {
		result += L" ";	
		if (_children[j]->isTerminal()) {
			result += _children[j]->_tag.to_string(); 
		}
		else {
			result += L"\n";
			int current_indent = indent;
			if (j == _head_index) current_indent--;
			for (int i = 0; i < current_indent; i++)
				result += L" ";
			if (j == _head_index) result += L"*";
			result += _children[j]->toPrettyParseWithMentions(ment1, label1, ment2, label2, indent);
		}
	}

	result += L")";

	return result;
}

void SynNode::dump(std::ostream &out, int indent) const {
	out << toDebugString(indent);
}


void SynNode::updateObjectIDTable() const {
	ObjectIDTable::addObject(this);

	for (int i = 0; i < _n_children; i++)
		_children[i]->updateObjectIDTable();
}

#define BEGIN_SYNNODE (reinterpret_cast<const wchar_t*>(StateLoader::IntegerCompressionStart + StateLoader::SynNodeOffset))

void SynNode::saveState(StateSaver *stateSaver) const {
	if (ParamReader::isParamTrue("use_state_file_integer_compression")) {
		stateSaver->beginList(BEGIN_SYNNODE, this);
	} else {
		stateSaver->beginList(L"SynNode", this);
	}

	stateSaver->saveInteger(_ID);
	stateSaver->saveSymbol(_tag);

	stateSaver->saveInteger(_start_token);
	stateSaver->saveInteger(_end_token);

	stateSaver->saveInteger(_mention_index);

	stateSaver->savePointer(_parent);
	stateSaver->saveInteger(_n_children);
	stateSaver->saveInteger(_head_index);
	stateSaver->beginList();
	for (int i = 0; i < _n_children; i++)
		_children[i]->saveState(stateSaver);
	stateSaver->endList();

	stateSaver->endList();
}

SynNode::SynNode(StateLoader *stateLoader) {
	//Use the integer replacement for "SynNode" if the state file was compressed
	int id = stateLoader->beginList(stateLoader->useCompressedState() ? BEGIN_SYNNODE : L"SynNode");
	stateLoader->getObjectPointerTable().addPointer(id, this);

	_ID = stateLoader->loadInteger();
	_tag = stateLoader->loadSymbol();

	_start_token = stateLoader->loadInteger();
	_end_token = stateLoader->loadInteger();

	_mention_index = stateLoader->loadInteger();

	_parent = (SynNode *) stateLoader->loadPointer();
	_n_children = stateLoader->loadInteger();
	_head_index = stateLoader->loadInteger();
	_children = _new SynNode*[_n_children];
	stateLoader->beginList();
	for (int i = 0; i < _n_children; i++)
		_children[i] = _new SynNode(stateLoader);
	stateLoader->endList();

	stateLoader->endList();
}

void SynNode::resolvePointers(StateLoader * stateLoader) {
	_parent = (SynNode *) stateLoader->getObjectPointerTable().getPointer(_parent);

	for (int i = 0; i < _n_children; i++)
		_children[i]->resolvePointers(stateLoader);
}

const wchar_t* SynNode::XMLIdentifierPrefix() const {
	return L"node";
}

void SynNode::saveXML(SerifXML::XMLTheoryElement synnodeElem, const Theory *context) const {
	using namespace SerifXML;
	const TokenSequence *tokSeq = dynamic_cast<const TokenSequence*>(context);
	if (context == 0)
		throw InternalInconsistencyException("SynNode::saveXML", "Expected context to be a TokenSequence");

	synnodeElem.setAttribute(X_tag, _tag);
	const Token *startTok = tokSeq->getToken(_start_token);
	synnodeElem.saveTheoryPointer(X_start_token, startTok);
	const Token *endTok = tokSeq->getToken(_end_token);
	synnodeElem.saveTheoryPointer(X_end_token, endTok);
	// Child nodes
	for (int i = 0; i < _n_children; ++i) {
		XMLTheoryElement childElem = synnodeElem.saveChildTheory(X_SynNode, _children[i], tokSeq);
		if (i == _head_index)
			childElem.setAttribute(X_is_head, X_TRUE);
	}
	// * Don't record mention_index -- it's redundant.
	// * Don't recored _ID -- it's just a depth-first identifier within this parse tree, and
	//   can be reconstructed at load time.
}

// Note: node_id_counter is an in-out parameter, used to assign a unique identifier
// to each SynNode, starting with zero at the root of the tree, and increasing by
// one for each node in a depth first traversal.  It is used to pick a value for _ID.
SynNode::SynNode(SerifXML::XMLTheoryElement synNodeElem, SynNode* parent, int &node_id_counter)
: _parent(parent), _head_index(-1), _mention_index(-1)
{
	using namespace SerifXML;
	synNodeElem.loadId(this);
	_tag = synNodeElem.getAttribute<Symbol>(X_tag);
	_ID = node_id_counter++;
	
	XMLTheoryElementList childElems = synNodeElem.getChildElementsByTagName(X_SynNode);
	_n_children = static_cast<int>(childElems.size());
	_children = _new SynNode*[_n_children];
	for (int i=0; i<_n_children; ++i) {
		_children[i] = _new SynNode(childElems[i], this, node_id_counter);
		if (childElems[i].getAttribute<bool>(X_is_head, false))
			_head_index = i;
	}
	if (_head_index == -1) {
		synNodeElem.reportLoadWarning("No head element found");
		// Todo: apply default head-finding rules??
	}

	XMLSerializedDocTheory *xmldoc = synNodeElem.getXMLSerializedDocTheory();
	_start_token = static_cast<int>(xmldoc->lookupTokenIndex(synNodeElem.loadTheoryPointer<Token>(X_start_token)));
	_end_token = static_cast<int>(xmldoc->lookupTokenIndex(synNodeElem.loadTheoryPointer<Token>(X_end_token)));
}

namespace { // private symbols
	const Symbol DATE_NNP_sym  (L"DATE-NNP");
	const Symbol DET_NN_sym    (L"DET:NN");
	const Symbol DET_NNS_sym   (L"DET:NNS");
	const Symbol DET_NNP_sym   (L"DET:NNP");
	const Symbol DET_NNPS_sym  (L"DET:NNPS");
	const Symbol IP_sym        (L"IP");
	const Symbol NN_sym        (L"NN");
	const Symbol NNS_sym       (L"NNS");
	const Symbol NNP_sym       (L"NNP");
	const Symbol NNPS_sym      (L"NNPS");
	const Symbol NR_sym        (L"NR");
	const Symbol NT_sym        (L"NT");
	const Symbol S_sym         (L"S");
	const Symbol SBAR_sym      (L"SBAR");
	const Symbol VB_sym        (L"VB");
	const Symbol VBG_sym       (L"VBG");
	const Symbol VBD_sym       (L"VBD");
	const Symbol VBN_sym       (L"VBN");
	const Symbol VBP_sym       (L"VBP");
	const Symbol VBZ_sym       (L"VBZ");
	const Symbol VA_sym        (L"VA");
	const Symbol VC_sym        (L"VC");
	const Symbol VE_sym        (L"VE");
	const Symbol VV_sym        (L"VV");
}

bool SynNode::isVerbTag(const Symbol& tag, LanguageAttribute language) {
	if (language == Language::ENGLISH) {
		return ((tag == VB_sym) || (tag == VBD_sym) ||
			    (tag == VBG_sym) || (tag == VBN_sym)|| 
			    (tag == VBP_sym) ||(tag == VBZ_sym));
	} else if (language == Language::CHINESE) {
		return ((tag == VA_sym) || (tag == VC_sym) || 
			    (tag == VE_sym) || (tag == VV_sym));
	} else {
		std::cerr << "Warning calling isVerbTag with unsupported language" << std::endl;
		return false;
	}
}

bool SynNode::isNounTag(const Symbol& tag, LanguageAttribute language) {
	if (language == Language::ENGLISH) {
		return ((tag == NN_sym)  || (tag == NNS_sym) ||
		        (tag == NNP_sym) || (tag == NNPS_sym) || 
		        (tag == DATE_NNP_sym));	
	} else if (language == Language::CHINESE) {
		return ((tag == NN_sym) || (tag == NR_sym) || (tag == NT_sym));
	} else if (language == Language::ARABIC) {
		return ((tag == NN_sym)      || (tag == NNS_sym) ||
		        (tag == NNP_sym)     || (tag == NNPS_sym) ||
		        (tag == DET_NN_sym)  || (tag == DET_NNS_sym) ||
		        (tag == DET_NNP_sym) || (tag == DET_NNPS_sym));
	} else {
		throw InternalInconsistencyException("SynNode::isNounTag", "Unsuported language");
	}
}

bool SynNode::isSentenceLikeTag(const Symbol& tag, LanguageAttribute language) {
	if (language == Language::ENGLISH) {
		return ((tag == S_sym) || (tag == SBAR_sym));	
	} else if (language == Language::CHINESE) {
		return ((tag == IP_sym));
	} else if (language == Language::ARABIC) {
		return ((tag == S_sym) || (tag == SBAR_sym));  // Is this right?
	} else {
		throw InternalInconsistencyException("SynNode::isisSentenceLikeTag", "Unsupported language");
	}
}
