// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/BridgeCollector.h"
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTrainer/ChainFinder.h"
#include "Generic/parse/ParserTrainer/TrainerKernelTable.h"
#include "Generic/parse/ParserTrainer/TrainerExtensionTable.h"
#include <cstddef>
#include <string>
using namespace std;
BridgeCollector::BridgeCollector()
{
	kernelTable = _new TrainerKernelTable(INITIAL_KERNEL_TABLE_SIZE);
	extensionTable = _new TrainerExtensionTable(INITIAL_EXTENSION_TABLE_SIZE);
}

void BridgeCollector::collect(ParseNode* parse)
{
	wstring result_chain = L"";
	ParseNode* nextNode = ChainFinder::find(parse, result_chain);

	if (nextNode->headNode->headNode != 0) {
		get_bridges(nextNode, nextNode->premods, BRANCH_DIRECTION_LEFT);
		get_bridges(nextNode, nextNode->postmods, BRANCH_DIRECTION_RIGHT);
		if (nextNode->premods != 0 || nextNode->postmods != 0) {
			collect(nextNode->headNode);
		}
	}

}

void BridgeCollector::get_bridges(ParseNode* head, ParseNode* modifier, int direction)
{
	//Note: should this be language specific
	Symbol previous = Symbol(L":ADJ");

	if (modifier != 0) {
		wstring head_string = L"";
		wstring modifier_string = L"";
		ParseNode* head_base = ChainFinder::find(head->headNode, head_string);
		ParseNode* modifier_base = ChainFinder::find(modifier, modifier_string);
	
		kernelTable->add(direction, head->label, head_base->label, 
			modifier_base->label, Symbol(head_string.c_str()), 
			head->headNode->label, Symbol(modifier_string.c_str()),
			modifier->label, modifier_base->headWordNode->label);

	
		while (modifier != 0) {
			collect(modifier);
			modifier_string = L"";
			modifier_base = ChainFinder::find(modifier, modifier_string);
			extensionTable->add(direction, head->label, head->headNode->label,
				previous, modifier_base->label, Symbol(modifier_string.c_str()),
				modifier->label, modifier_base->headWordNode->label);
			previous = modifier->label;
			modifier = modifier->next;
		}
	}

	Symbol exit_symbol = Symbol(L":EXIT");
	extensionTable->add(direction, head->label, head->headNode->label, previous,
		 exit_symbol, exit_symbol, exit_symbol, exit_symbol); 

}


void BridgeCollector::print_all (char* kernel_file, char* extension_file)
{
	kernelTable->print(kernel_file);
	extensionTable->print(extension_file);
}
