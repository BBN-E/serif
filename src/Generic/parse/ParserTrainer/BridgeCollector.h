// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BRIDGE_COLLECTOR_H
#define BRIDGE_COLLECTOR_H

#include <cstddef>
#include "Generic/parse/ParseNode.h"
#include "Generic/parse/ParserTrainer/TrainerKernelTable.h"
#include "Generic/parse/ParserTrainer/TrainerExtensionTable.h"

#define INITIAL_KERNEL_TABLE_SIZE 9000
#define INITIAL_EXTENSION_TABLE_SIZE 40000

class BridgeCollector
{
private:
	TrainerKernelTable* kernelTable;
	TrainerExtensionTable* extensionTable;
	
	void get_bridges(ParseNode* nextNode, ParseNode* modifier, int direction);

public:
	BridgeCollector();
	void collect(ParseNode* parse);
	void print_all (char* kernel_file, char* extension_file);

};


#endif
