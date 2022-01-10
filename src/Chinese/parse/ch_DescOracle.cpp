// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Chinese/parse/ch_DescOracle.h"
#include "Generic/parse/ParserTags.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

ChineseDescOracle::ChineseDescOracle(KernelTable* kernelTable, 
					   ExtensionTable* extensionTable,
					   const char *inventoryFile) 
{
	
	_npChains = _new SymbolHash(1000);

	ExtensionTable::iterator i;
	for (i = extensionTable->table->begin(); i != extensionTable->table->end(); ++i) {
		int numExtensions = (*i).second.length;
		BridgeExtension* be = (*i).second.data;
		for (int j = 0; j < numExtensions; j++) {
			wstring s_string = be[j].modifierChain.to_string();
			
			if (s_string.find(L"=NP") != wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0)
				_npChains->add(be[j].modifierChain);
			
        }
	}

	KernelTable::iterator k;
	for (k = kernelTable->table->begin(); k != kernelTable->table->end(); ++k) {
		int numKernels = (*k).second.length;
		BridgeKernel* bk = (*k).second.data;
		for (int j = 0; j < numKernels; j++) {
            wstring s_string = bk[j].headChain.to_string();

			if (s_string.find(L"=NP") != wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0)
				_npChains->add(bk[j].headChain);
			
				
			s_string = bk[j].modifierChain.to_string();
			if (s_string.find(L"=NP") != wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0 )
				_npChains->add(bk[j].modifierChain);
        }
	}

	readInventory(inventoryFile);
}

void ChineseDescOracle::readInventory(const char *filename) {
	_significantHeadWords = _new SymbolHash(1000);

	if (filename[0] != L'\0') {
		boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& stream(*stream_scoped_ptr);
		stream.open(filename);
		UTF8Token token;
		int count;

		while (!stream.eof()) {
			stream >> token; // word
			_significantHeadWords->add(token.symValue());
			//cout << token.chars() << " ";

			stream >> count; // type count

			for (int i = 0; i < count; i++) {
				stream >> token; // type
			}
		}
		stream.close();
	}
}

bool ChineseDescOracle::isPossibleDescriptor(ChartEntry *entry, Symbol chain) const {

	
	if (isNounPhrase(entry, chain)) {

		// ALL NPs (except possessives in English)
		if (entry->headIsSignificant)
			return true;
		else return false;

	} else return false;

}

bool ChineseDescOracle::isPossibleDescriptorHeadWord(Symbol word) const {
	if (_significantHeadWords->lookup(word))
		return true;
	return false;	
}


bool ChineseDescOracle::isNounPhrase(ChartEntry *entry, Symbol chain) const {
	if (entry->constituentCategory == ChineseSTags::NP ||
		entry->constituentCategory == ChineseSTags::NPA ||
		_npChains->lookup(chain))
		return true;
	else return false;
}
