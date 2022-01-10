// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/parse/ParserTrainer/TrainerExtensionTable.h"
#include "Generic/parse/ParserTrainer/LinkedBridgeExtension.h"
#include "Generic/parse/ExtensionKey.h"
#include "Generic/common/UTF8OutputStream.h"

const float TrainerExtensionTable::targetLoadingFactor = static_cast<float>(0.7);

// cc = constituentCategory
// hcat = headCategory
// prev = previousModifierCategory
// mbc = modifierBaseCategory
// mc = modifierChain
// mcf = modifierChainFront
// mt = modifierTag
void TrainerExtensionTable::add(int direction, Symbol cc, Symbol hcat, 
								Symbol prev, Symbol mbc,
								Symbol mc, Symbol mcf, Symbol mt)
{
	ExtensionKey key (direction, cc, hcat, mbc, prev, mt);
	Table::iterator iter; 


    iter = table->find(key);
    if (iter == table->end()) {
		LinkedBridgeExtension* lbe = 
			_new LinkedBridgeExtension(direction, cc, hcat, prev, 
										mbc, mc, mcf, mt);
		(*table)[key] = lbe;
		size++;
    }
    else {
		LinkedBridgeExtension* list = (*iter).second;
		while (true) {
			if (compare_extension(list->extension, direction, cc, 
									hcat, prev, mbc, mc, mcf, mt))
				break;
			if (list->next == 0) {
				list->next = 
					_new LinkedBridgeExtension(direction, cc, hcat, 
												prev, mbc, mc, mcf, mt);
				break;
			}
			list = list->next;
		}

	}

	return;
}

bool TrainerExtensionTable::compare_extension(BridgeExtension e, 
					int direction, Symbol cc, Symbol hcat, Symbol prev, 
					Symbol mbc, Symbol mc, Symbol mcf, Symbol mt)
{
	if (e.branchingDirection == direction && e.constituentCategory == cc &&
		e.headCategory == hcat && e.previousModifierCategory == prev &&
		e.modifierBaseCategory == mbc && e.modifierChain == mc &&
		e.modifierChainFront == mcf && e.modifierTag == mt)
		return true;
	else return false;
}

void TrainerExtensionTable::print(char *filename)
{

	UTF8OutputStream out;
	out.open(filename);

	Table::iterator iter;

	out << size;
	out << '\n';
	
	for (iter = table->begin() ; iter != table->end() ; ++iter) {
		out << "((";
		ExtensionKey key = (*iter).first;
		if (key.branchingDirection == BRANCH_DIRECTION_LEFT)
			out << "LEFT ";
		else out << "RIGHT ";
		out << key.constituentCategory.to_string() << ' ';
		out << key.headCategory.to_string() << ' ';
		out << key.modifierBaseCategory.to_string() << ' ';
		out << key.previousModifierCategory.to_string() << ' ';
		out << key.modifierTag.to_string();
		out << ") ";
		LinkedBridgeExtension* value = (*iter).second;
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
			if (value->extension.branchingDirection == BRANCH_DIRECTION_LEFT)
				out << "LEFT ";
			else out << "RIGHT ";
			out << value->extension.constituentCategory.to_string() << ' ';
			out << value->extension.headCategory.to_string() << ' ';
			out << value->extension.previousModifierCategory.to_string() << ' ';
			out << value->extension.modifierBaseCategory.to_string() << ' ';
			out << value->extension.modifierChain.to_string() << ' ';
			out << value->extension.modifierChainFront.to_string() << ' ';
			out << value->extension.modifierTag.to_string() << ')';
			value = value->next;
		}
		out << ")\n";
	}

	out.close();

	return;
}
