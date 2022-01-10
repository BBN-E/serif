// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/parse/ParserTrainer/TrainerKernelTable.h"
#include "Generic/parse/ParserTrainer/LinkedBridgeKernel.h"
#include "Generic/parse/KernelKey.h"
#include "Generic/common/UTF8OutputStream.h"


const float TrainerKernelTable::targetLoadingFactor = static_cast<float>(0.7);

// cc = constituentCategory
// hbc = headBaseCategory
// mbc = modifierBaseCategory
// hc = headChain
// hcf = headChainFront
// mc = modifierChain
// mcf = modifierChainFront
// mt = modifierTag
void TrainerKernelTable::add(int direction, Symbol cc, Symbol hbc, 
							 Symbol mbc, Symbol hc, Symbol hcf, Symbol mc, 
							 Symbol mcf, Symbol mt)
{
	KernelKey key (direction, hbc, mbc, mt);
	
	Table::iterator iter; 

    iter = table->find(key);
    if (iter == table->end()) {
		LinkedBridgeKernel* lbk = 
			_new LinkedBridgeKernel(direction, cc, hbc, mbc, 
									hc, hcf, mc, mcf, mt);
		(*table)[key] = lbk;
		size++;
    }
    else {
		LinkedBridgeKernel* list = (*iter).second;
		while (true) {
			if (compare_kernel(list->kernel, direction, cc, hbc, mbc, 
							   hc, hcf, mc, mcf, mt))
				break;
			if (list->next == 0) {
				list->next = 
					_new LinkedBridgeKernel(direction, cc, hbc, mbc, 
										   hc, hcf, mc, mcf, mt);
				break;
			}
			list = list->next;
		}

	}

	return;
}

bool TrainerKernelTable::compare_kernel(BridgeKernel k, int direction, 
							Symbol cc, Symbol hbc, Symbol mbc, Symbol hc, 
							Symbol hcf, Symbol mc, Symbol mcf, Symbol mt)
{
	if (k.branchingDirection == direction && k.constituentCategory == cc &&
		k.headBaseCategory == hbc && k.headChain == hc && 
		k.headChainFront == hcf && k.modifierBaseCategory == mbc && 
		k.modifierChain == mc && k.modifierChainFront == mcf && 
		k.modifierTag == mt)
		return true;
	else return false;
}

void TrainerKernelTable::print(char *filename)
{

	UTF8OutputStream out;
	out.open(filename);

	out << size;
	out << '\n';

	Table::iterator iter;

	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << "((";
		KernelKey key = (*iter).first;
		if (key.branchingDirection == BRANCH_DIRECTION_LEFT)
			out << "LEFT ";
		else out << "RIGHT ";
		out << key.headBaseCategory.to_string() << ' ';
		out << key.modifierBaseCategory.to_string() << ' ';
		out << key.modifierTag.to_string();
		out << ") ";
		LinkedBridgeKernel* value = (*iter).second;
		int count = 1;
		while (value->next != 0)
		{
			value = value->next;
			count++;
		}
		out << count;
		value = (*iter).second;
		while (value != 0) {
			out << " (";
			if (value->kernel.branchingDirection == BRANCH_DIRECTION_LEFT)
				out << "LEFT ";
			else out << "RIGHT ";
			out << value->kernel.constituentCategory.to_string() << ' ';
			out << value->kernel.headBaseCategory.to_string() << ' ';
			out << value->kernel.modifierBaseCategory.to_string() << ' ';
			out << value->kernel.headChain.to_string() << ' ';
			out << value->kernel.headChainFront.to_string() << ' ';
			out << value->kernel.modifierChain.to_string() << ' ';
			out << value->kernel.modifierChainFront.to_string() << ' ';
			out << value->kernel.modifierTag.to_string() << ')';
			value = value->next;
		}
		out << ")\n";
	}

	out.close();

	return;
}
