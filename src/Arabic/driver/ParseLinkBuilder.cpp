// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/driver/ParseLinkBuilder.h"
#include "Generic/common/LocatedString.h"
#include "Generic/theories/NameSpan.h"
#include "Generic/theories/EntityConstants.h"
#include "Generic/theories/Entity.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTags.h"
#include <stdlib.h>
#include <stdio.h>




ParseLinkBuilder::ParseLinkBuilder(){
	_name_count = 0;
	_word_count = 0;
	_nnp_count = 0;
	_top_node_count = 0;
	_start_tag = 0;
	_start_loc = 0;
	//_begin_list = _new int[MAX_NAMES];
	//_end_list=_new int[MAX_NAMES];
	//_type_list = _new Entity::Type[MAX_NAMES];
	//_coref_list = _new int[MAX_NAMES];
	//_tags = _new Symbol[6];
	//index ent id lookup
	//_per_to_ent_id = _new int[MAX_ENTITIES];
	//_org_to_ent_id = _new int[MAX_ENTITIES];
	//_gpe_to_ent_id = _new int[MAX_ENTITIES];
	//_loc_to_ent_id = _new int[MAX_ENTITIES];
	//_fac_to_ent_id = _new int[MAX_ENTITIES];
	_num_string = _new wchar_t[20];

	//_nodeIDGenerator =IDGenerator(0);
	_nodeID=0;
	//use just the begining to find types, since end maybe :coref
	_tags[OTH]=  Symbol(L"</ENAMEX>:"); //add : for truncating
	_tags[PER] = Symbol(L"<ENAMEX TYPE=\"PERSON:");
	_tags[ORG] = Symbol(L"<ENAMEX TYPE=\"ORGANIZATION:");
	_tags[LOC] = Symbol(L"<ENAMEX TYPE=\"LOCATION:");
	_tags[FAC] = Symbol(L"<ENAMEX TYPE=\"FACILITY:");
	_tags[GPE] = Symbol(L"<ENAMEX TYPE=\"GPE:");
	_tree = _new ParseNode(ParserTags::FRAGMENTS);
	_placeholder = _tree;
	for(int i =0; i<MAX_ENTITIES; i++){
		_per_to_ent_id[i] = -1;
		_org_to_ent_id[i] = -1;
		_gpe_to_ent_id[i] = -1;
		_loc_to_ent_id[i] = -1;
		_fac_to_ent_id[i] = -1;
	}
}
void ParseLinkBuilder::Reset(){
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
	delete _tree;
	_tree =_new ParseNode(ParserTags::FRAGMENTS);
	_placeholder = _tree;
}
ParseLinkBuilder::~ParseLinkBuilder(){
//	delete _tags;
//	delete _type_list;
	delete _tree;
//	delete _begin_list;
//	delete _end_list;
//	delete _num_string;
//	delete _per_to_ent_id;
//	delete _org_to_ent_id;
//	delete _gpe_to_ent_id;
//	delete _loc_to_ent_id;
//	delete _fac_to_ent_id;
}

//COPIED ENTIRE PRIVATE FUNCTION FROM PARSER.CPP
SynNode* ParseLinkBuilder::convertParseToSynNode(ParseNode* parse, SynNode* parent, 
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

	
void ParseLinkBuilder::addWord(Symbol sym){
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

ParseNode* ParseLinkBuilder::addNode(int count, ParseNode* result){
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

ParseNode* ParseLinkBuilder::addLevel(int count, ParseNode* result){
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
int ParseLinkBuilder::addTag(int thisTag){
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
		//Names are not embedded - will correspond with _begin_list and _end_list
		_coref_list[_name_count] = _curr_ref;		
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
int ParseLinkBuilder::processTag(Symbol symtag){
	int thisTag =-1;
	const wchar_t* tagString = symtag.to_string();
	int inEnd = 0;
	int link =-1;
	while(true){
		if(tagString[inEnd]==L':'){
			int i = 0;
			int j =inEnd+1;
			while(tagString[j]!=L'>'){
				_num_string[i] = tagString[j];
				i++;
				j++;
			}
			_num_string[i]=L'\0';
			//does this leak _stop_string?
			link = std::wcstol(_num_string, &_stop_string, 10);			
			break;
		}
		if(tagString[inEnd]==L'>')
			break;
		inEnd++;
	}
	for(int i=0; i<6; i++){
		const wchar_t* matchString =_tags[i].to_string();
		int j = 0;
		int isMatch = 1;
		while(j <inEnd && (matchString[j] != L':')){
			if(tagString[j]==matchString[j])
				j++;
			else{
				isMatch = 0;
				break;
			}
		}
		if(isMatch){
			//Check that this was the end of tagString
			if((tagString[j]==L'\"')||(tagString[j]==L':')||(tagString[j]==L'>'))
				thisTag =i;
			break;
		}
	}
	if(thisTag > -1){
		_curr_ref = link;
	}
	return thisTag;
};

void ParseLinkBuilder::getNameSpan(NameSpan* &ns, int i){
	ns = _new NameSpan();
	ns->start = _begin_list[i];
	ns->end = _end_list[i];
	ns->type =_type_list[i];
};

void ParseLinkBuilder::buildEntitySet(const MentionSet* ments, EntitySet* ents){
	
	for (int i = 0; i <_name_count; i++) {
		int start = _begin_list[i];
		int end = _end_list[i];
		Entity::Type etype = _type_list[i];
		int id =_coref_list[i];
		bool assigned = false;
		Mention *mention;
		for (int j = 0; j < ments->getNMentions(); j++) {
			mention = ments->getMention(j);
			if (mention->node->getStartToken() == start &&
				mention->node->getEndToken() == end)
			{
				assigned = true;
				break;
			}
		}
		/*
		if (!assigned)
			throw InternalInconsistencyException("NameTheory::putInMentionSet()", "NameLink has no corresponding parse node!");
		*/
		//Unlinked entity
		if (id == -1){
			ents->addNew(mention, etype);
			//_debugOut << "\n CREATED ENTITY #" << newSet->getNEntities()-1;
		}
		//Look to see if this type has been added
		
		else {
			int eid;
			int* lookup; 
			if(etype == EntityConstants::GPE)
				lookup = _gpe_to_ent_id;
			else if(etype == EntityConstants::PER)
				lookup = _per_to_ent_id;
			else if(etype == EntityConstants::ORG)
				lookup = _org_to_ent_id;
			else if(etype == EntityConstants::FAC)
				lookup = _fac_to_ent_id;
			else if(etype == EntityConstants::LOC)
				lookup = _loc_to_ent_id;
			eid = lookup[id];
			if(eid == -1){
				ents->addNew(mention, etype);
				const Entity* thisEnt = ents->getEntity(mention);
				lookup[id]=thisEnt->ID;
			}
			else{
				ents->add(mention, eid);
			}
		}
	}
}

	

