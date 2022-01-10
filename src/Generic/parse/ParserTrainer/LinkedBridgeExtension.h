// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINKED_BRIDGE_EXTENSION_H
#define LINKED_BRIDGE_EXTENSION_H

#include "Generic/parse/BridgeExtension.h"

class LinkedBridgeExtension
{
public:
	BridgeExtension extension;
	LinkedBridgeExtension* next;

	LinkedBridgeExtension(int direction, Symbol cc, Symbol hcat, 
			Symbol prev, Symbol mbc, Symbol mc, Symbol mcf, Symbol mt) {

		extension.branchingDirection = direction; 
		extension.constituentCategory = cc;
		extension.headCategory = hcat; 
		extension.modifierBaseCategory = mbc; 
		extension.modifierChain = mc;
		extension.modifierChainFront = mcf; 
		extension.modifierTag = mt; 
		extension.previousModifierCategory = prev;
		next = 0;
	}
	
	LinkedBridgeExtension() {
		next = 0;
	}
};

    
#endif
