// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "common/DescriptorInventory.h"
#include "common/UnexpectedInputException.h"
#include "common/SymbolUtilities.h"
#include "common/SymbolConstants.h"
#include "common/Sexp.h"
#include "common/UTF8InputStream.h"
#include <boost/scoped_ptr.hpp>


DescriptorInventory::DescriptorInventory() {
	_inventoryMap = _new InventoryMap(100, hasher, eqTester);
}

DescriptorInventory::DescriptorInventory(const char *filename) {
	_inventoryMap = 0;
	loadInventory(filename);
}

void DescriptorInventory::loadInventory(const char *filename) {

	if (_inventoryMap == 0)
		_inventoryMap = _new InventoryMap(100, hasher, eqTester);

	boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& in(*in_scoped_ptr);
	in.open(filename);
	Sexp *sexp = _new Sexp(in);
	while (!sexp->isVoid()) {
		int nkids = sexp->getNumChildren();
		if (nkids != 3 && nkids != 4)
			throwFormatException(sexp);
		if (!sexp->getFirstChild()->isAtom() || 
			!sexp->getSecondChild()->isList() ||
			!sexp->getThirdChild()->isList())
			throwFormatException(sexp);
		
				
		Symbol word = sexp->getFirstChild()->getValue();
		Sexp *synset = sexp->getSecondChild();
		Sexp *referent = sexp->getThirdChild();
		if (referent->getNumChildren() != 2 || !referent->getSecondChild()->isAtom() ||
			synset->getNumChildren() != 2 || !synset->getSecondChild()->isList())
			throwFormatException(sexp);

		synset = synset->getSecondChild();

		InventoryEntry *entry;
		try {
			EntityType etype(referent->getSecondChild()->getValue());
			entry = _new InventoryEntry();		
			entry->etype = etype;
		} catch (UnexpectedInputException) {
			delete sexp;
			sexp = _new Sexp(in);
			continue;
		}
		
		entry->nrels = 0;
		if (nkids > 3) {
			Sexp *relations = sexp->getNthChild(3);
			if (!relations->isList())
				throwFormatException(sexp);
			int nrels = relations->getNumChildren() - 1;
			if (nrels <= 0)
				throwFormatException(sexp);
			entry->relations = _new RelationInventoryEntry[nrels];
			entry->nrels = nrels;
			for (int i = 0; i < nrels; i++) {
				Sexp *relSexp = relations->getNthChild(i+1);
				if (!relSexp->isList() || relSexp->getNumChildren() != 3)
					throwFormatException(sexp);
				Symbol relType = relSexp->getFirstChild()->getValue();
				if (relSexp->getSecondChild()->getValue() == Symbol(L"ref")) {
					try {
						EntityType otherType(relSexp->getThirdChild()->getValue());
						entry->relations[i].otherEntityType = otherType;
					} catch (UnexpectedInputException) {
						entry->relations[i].otherEntityType = EntityType::getUndetType();
					}
					entry->relations[i].relType = relType;
				} else {
					try {
						EntityType otherType(relSexp->getSecondChild()->getValue());
						entry->relations[i].otherEntityType = otherType;
					} catch (UnexpectedInputException) {
						entry->relations[i].otherEntityType = EntityType::getUndetType();
					}
					std::wstring str = relType.to_string();
					str += L":reversed";
					entry->relations[i].relType = Symbol(str.c_str());

				}
			}
		} else entry->relations = 0;

		 (*_inventoryMap)[word] = entry;
		 if (!synset->isList())
			throwFormatException(sexp);
		 int n_syns = synset->getNumChildren();
		 for (int syn = 0; syn < n_syns; syn++) {
			 Symbol synonym = synset->getNthChild(syn)->getValue();
			 (*_inventoryMap)[synonym] = entry;
		 }
		 delete sexp;
		 sexp = _new Sexp(in);
	}

}

Symbol DescriptorInventory::getDescType(Symbol word) {
	InventoryMap::iterator iter;

	Symbol stemmedWord = SymbolUtilities::stemDescriptor(word);
    iter = _inventoryMap->find(stemmedWord);
    if (iter == _inventoryMap->end()) {
		return SymbolConstants::nullSymbol;
    }
    InventoryEntry *entry = (*iter).second;
	return entry->etype.getName();
}

Symbol DescriptorInventory::getRelationType(Symbol stemmedPredicate, 
											EntityType refType, EntityType otherType) 
{
	InventoryMap::iterator iter;

    iter = _inventoryMap->find(stemmedPredicate);
    if (iter == _inventoryMap->end()) {
		return SymbolConstants::nullSymbol;
    }
    InventoryEntry *entry = (*iter).second;
	if (entry->etype != refType)
		return SymbolConstants::nullSymbol;
	for (int i = 0; i < entry->nrels; i++) {
		if (entry->relations[i].otherEntityType == otherType)
			return entry->relations[i].relType;
	}
	return SymbolConstants::nullSymbol;
}

void DescriptorInventory::throwFormatException(Sexp *sexp) {
	char message[2000];
	_snprintf(message, 2000, "Ill-formed sexp (%s) in noun inventory",
		sexp->to_debug_string());
	throw UnexpectedInputException("DescriptorInventory::throwFormatException", message);
}


