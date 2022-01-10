// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINKED_BRIDGE_KERNEL_H
#define LINKED_BRIDGE_KERNEL_H

#include "Generic/parse/BridgeKernel.h"

class LinkedBridgeKernel
{
public:
	BridgeKernel kernel;
	LinkedBridgeKernel* next;

	LinkedBridgeKernel(int direction, Symbol cc, Symbol hbc, 
			Symbol mbc, Symbol hc, Symbol hcf, Symbol mc, 
			Symbol mcf, Symbol mt) {

		kernel.branchingDirection = direction; 
		kernel.constituentCategory = cc;
		kernel.headBaseCategory = hbc; 
		kernel.headChain = hc; 
		kernel.headChainFront = hcf;
		kernel.modifierBaseCategory = mbc; 
		kernel.modifierChain = mc;
		kernel.modifierChainFront = mcf; 
		kernel.modifierTag = mt;
		next = 0;
	}
		
	LinkedBridgeKernel () {
		next = 0;
	}
};

    
#endif
