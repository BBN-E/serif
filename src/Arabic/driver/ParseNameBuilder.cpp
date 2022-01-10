// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/driver/ParseNameBuilder.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/EntityConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTags.h"


ParseNameBuilder::ParseNameBuilder(){
	_name_count = 0;
	_word_count = 0;
	_nnp_count = 0;
	_top_node_count = 0;
	_start_tag = 0;
	_start_loc = 0;
	//_begin_list = _new int[MAX_NAMES];
	//_end_list=_new int[MAX_NAMES];
	//_type_list = Entity::Type[MAX_NAMES];
	//_tags = _new Symbol[6];
	_nodeID=0;

	_tags[OTH]=  Symbol(L"</ENAMEX>");
	_tags[PER] = Symbol(L"<ENAMEX TYPE=\"PERSON\">");
	_tags[ORG] = Symbol(L"<ENAMEX TYPE=\"ORGANIZATION\">");
	_tags[LOC] = Symbol(L"<ENAMEX TYPE=\"LOCATION\">");
	_tags[FAC] = Symbol(L"<ENAMEX TYPE=\"FACILITY\">");
	_tags[GPE] = Symbol(L"<ENAMEX TYPE=\"GPE\">");

	_tree = _new ParseNode(ParserTags::FRAGMENTS);
	_placeholder = _tree;
}
void ParseNameBuilder::Reset(){
	_name_count = 0;
	_word_count = 0;
	_start_tag = 0;
	_start_loc = 0;
	_comp_name_count= 0;
	_top_node_count = 0;
	//_nodeIDGenerator =IDGenerator(0);
	_nodeID = 0;
	//need to delete old parse nodes
	//TODO:  Add heap memory use - does deleteing top, delete all?
	ParseNode* prev, place;
	delete _tree;
	_tree =_new ParseNode(ParserTags::FRAGMENTS);
	_placeholder = _tree;
}
ParseNameBuilder::~ParseNameBuilder(){
//	delete _tags;
//	delete _type_list;
	delete _tree;
//	delete _begin_list;
//	delete _end_list;
}
//COPIED ENTIRE PRIVATE FUNCTION FROM PARSER.CPP
SynNode* ParseNameBuilder::convertParseToSynNode(ParseNode* parse, SynNode* parent, 
									   int start_token)

{
	
	if (parse->headNode == 0) {
		SynNode *snode = _new SynNode(_nodeID++, parent, 
			parse->label, 0);
		snode->setTokenSpan(start_token, start_token);
		return snode;
	} 

	// count children, store last premod node
	int n_children = 0;
	ParseNode* iterator = parse->premods;
	ParseNode* last_premod = parse->premods;
	while (iterator != 0) {
		n_children++;
		last_premod = iterator;
		iterator = iterator->next;
	}
	int head_index = n_children;
	n_children++;
	iterator = parse->postmods;
	while (iterator != 0) {
		n_children++;
		iterator = iterator->next;
	}

	SynNode *snode = _new SynNode(_nodeID++, parent, 
		parse->label, n_children);
	snode->setHeadIndex(head_index);

	int token_index = start_token;

	// make life easier
	if (parse->premods != 0) {
		parse->premods_reverse(parse->premods, parse->premods->next);
		parse->premods->next = 0;
	}

	// create children
	int child_index = 0;
	iterator = last_premod;
	while (iterator != 0) {
		snode->setChild(child_index, 
			convertParseToSynNode(iterator, snode, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	snode->setChild(child_index, 
			convertParseToSynNode(parse->headNode, snode, token_index));
	token_index = snode->getChild(child_index)->getEndToken() + 1;	
	child_index++;
	iterator = parse->postmods;
	while (iterator != 0) {
		snode->setChild(child_index, 
			convertParseToSynNode(iterator, snode, token_index));
		iterator = iterator->next;
		token_index = snode->getChild(child_index)->getEndToken() + 1;			
		child_index++;
	}
	
	snode->setTokenSpan(start_token, token_index - 1);
	return snode;

};
//end of ArabicParser.cpp function

	
void ParseNameBuilder::addWord(Symbol sym){
	//Make a flat parse tree
	//Parse needs a head and post mods, so make first word head, all following postmods
	ParseNode* wordNode = _new ParseNode(sym);
	if(_start_tag == 0){
		ParseNode* result = _new ParseNode(Symbol(L"XX"));
		result->headNode = wordNode;
		_placeholder = addNode(_top_node_count, result);
		_top_node_count++;

	}
	else{
		ParseNode* result = _new ParseNode(Symbol(L"NNP"));
		result->headNode = wordNode;
		_placeholder = addNode(_nnp_count, result);
		_nnp_count++;
	}
	_word_count++;
}

ParseNode* ParseNameBuilder::addNode(int count, ParseNode* result){
		if(count==0){
			_placeholder->headNode=result;			
		}
		else if(count==1){

			_placeholder->postmods=result;
			_placeholder = _placeholder->postmods;
		}
		else{
			_placeholder->next=result;
			_placeholder = _placeholder->next;

		}
		return _placeholder;
}

ParseNode* ParseNameBuilder::addLevel(int count, ParseNode* result){
		if(count==0){
			_placeholder->headNode=result;
			_lastpostmod = _placeholder;
			_placeholder = result->headNode;
		}
		else if(count==1){

			_placeholder->postmods=result;
			_lastpostmod = _placeholder->postmods;
			_placeholder = _placeholder->postmods->headNode;
		}
		else{
			_placeholder->next=result;
			_lastpostmod = _placeholder->next;
			_placeholder = _placeholder->next->headNode;
		}
		return _placeholder;
}

//assume well formed xml
int ParseNameBuilder::addTag(int thisTag){
	//make a name theory for the word - since this is UTF8 data, can't use ne_decoder		
	if(thisTag>0){
		_start_tag = thisTag;
		_start_loc = _word_count;
		_nnp_count= 0;
		ParseNode *  oldPlace = _placeholder;
		//also make a new NPP node for parse tree- since name linker looks at the head
		//of the npp, make a head node for the npp  that will span the name
		ParseNode * npp =  _new ParseNode(Symbol(L"NPP"));
		ParseNode * nppc = _new ParseNode(Symbol(L"NPPCHILD"));
		npp->headNode = nppc;
		_placeholder = addLevel(_top_node_count, npp);
		
		_top_node_count++;

	}
	else if(thisTag==0){
		if(_start_tag <1){	//this can happen with bad sentence breaks
			_start_tag =0;
//			std::cerr<<"Bad Name input in XML file"<<std::endl;  
		}
		else{
			_begin_list[_name_count]=_start_loc;
			_end_list[_name_count]=_word_count-1;
			_type_list[_name_count] = EntityConstants::allEntityConstants[_start_tag];
			_name_count++;
			//move placeholder back to fragment child level
			_placeholder = _lastpostmod;
			_start_tag = 0;

		}
	}
	return thisTag;
};
int ParseNameBuilder::processTag(Symbol symtag){
	int thisTag =-1;
	for(int i=0; i<6; i++){
		if( symtag ==_tags[i]){
			thisTag =i;
			break;
		}
	}
	return thisTag;
};

void ParseNameBuilder::getNameSpan(NameSpan* &ns, int i){
	ns = _new NameSpan();
	ns->start = _begin_list[i];
	ns->end = _end_list[i];
	ns->type =_type_list[i];
};
