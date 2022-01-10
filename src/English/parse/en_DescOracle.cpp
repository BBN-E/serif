// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/parse/en_DescOracle.h"
#include "Generic/parse/ParserTags.h"
#include "English/parse/en_STags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"
#include "Generic/common/SymbolHash.h"
#include <boost/scoped_ptr.hpp>

using namespace std;

EnglishDescOracle::EnglishDescOracle(KernelTable* kernelTable, 
					   ExtensionTable* extensionTable,
					   const char *inventoryFile) {

	_NP_NPPOS_Symbol = Symbol (L"S=NPPOS");
	_S_NPPOS_Symbol = Symbol (L"NP=NPPOS");
	
	_npChains = _new SymbolHash(1000);
	_wordNet = WordNet::getInstance();

	ExtensionTable::iterator i;
	for (i = extensionTable->table->begin(); i != extensionTable->table->end(); ++i) {
		int numExtensions = (*i).second.length;
		BridgeExtension* be = (*i).second.data;
		for (int j = 0; j < numExtensions; j++) {
			wstring s_string = be[j].modifierChain.to_string();
			
			if (s_string.find(L"=NP") != std::wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0 ||
				s_string.find(L"NPPOS") == 0)
				_npChains->add(be[j].modifierChain);
			
        }
	}

	KernelTable::iterator k;
	for (k = kernelTable->table->begin(); k != kernelTable->table->end(); ++k) {
		int numKernels = (*k).second.length;
		BridgeKernel* bk = (*k).second.data;
		for (int j = 0; j < numKernels; j++) {
            wstring s_string = bk[j].headChain.to_string();

			if (s_string.find(L"=NP") != std::wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0 ||
				s_string.find(L"NPPOS") == 0)
				_npChains->add(bk[j].headChain);
			
				
			s_string = bk[j].modifierChain.to_string();
			if (s_string.find(L"=NP") != std::wstring::npos ||
				s_string.find(L"NP") == 0 ||
				s_string.find(L"NPA") == 0 ||
				s_string.find(L"NPPOS") == 0)
				_npChains->add(bk[j].modifierChain);
        }
	}

	readInventory(inventoryFile);

}



void EnglishDescOracle::readInventory(const char *filename) {
	
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build());
	UTF8InputStream& stream(*stream_scoped_ptr);
	stream.open(filename);
	_significantHeadWords = _new SymbolHash(1000);
	UTF8Token token;

	while (!stream.eof()) {
		stream >> token; // (
		if (token.symValue() != ParserTags::leftParen)
			break;
		stream >> token; // word
		_significantHeadWords->add(token.symValue());
		//cout << token.chars() << " ";

		stream >> token; // (
		stream >> token; // synsets
		stream >> token; // (
		while (true) {
			stream >> token; // word or )
			if (token.symValue() == ParserTags::rightParen) {
				stream >> token; // )
				break;
			}
			if (token.symValue() == ParserTags::leftParen) {
				std::string msg = "";
				msg += "In file: ";
				msg += filename;
				msg += "\nERROR: ill-formed synset";
				throw UnexpectedInputException("EnglishDescOracle::readInventory", msg.c_str());
			}
			_significantHeadWords->add(token.symValue());
		}

		stream >> token; // (
		stream >> token; // referent
		stream >> token; // type
		stream >> token; // )

		stream >> token; // ( or )
		if (token.symValue() == ParserTags::rightParen)
			continue;
		stream >> token; // relations
		stream >> token; // (
		while (true) {
			stream >> token; // type
			stream >> token; // arg1
			stream >> token; // arg2
			stream >> token; // )
			stream >> token; // ( or )
			if (token.symValue() == ParserTags::rightParen) {
				stream >> token; // )
				break;
			}
			if (token.symValue() != ParserTags::leftParen) {
				std::string msg = "";
				msg += "In file: ";
				msg += filename;
				msg += "\nERROR: ill-formed inventory record";
				throw UnexpectedInputException("EnglishDescOracle::readInventory", msg.c_str());
			}
		}
	}

	stream.close();
	
}

bool EnglishDescOracle::isPossibleDescriptor(ChartEntry *entry, Symbol chain) const {

	if (entry->constituentCategory == EnglishSTags::NPPOS ||
		chain == _NP_NPPOS_Symbol ||
		chain == _S_NPPOS_Symbol) {

		if (entry->isPreterminal)
			return false;

		// special fix for POSSESSIVES
		ChartEntry *entryIteratorParent = entry;
		ChartEntry *entryIterator = entry->rightChild;
		while (entryIterator->rightChild != 0) {
			entryIteratorParent= entryIterator;
			entryIterator = entryIterator->rightChild;
		}

		return entryIteratorParent->leftChild->headIsSignificant;
			
	} else if (isNounPhrase(entry, chain)) {
		// ALL OTHER NPs
		return entry->headIsSignificant;
	} else return false;

}

bool EnglishDescOracle::isPossibleDescriptorHeadWord(Symbol word) const 
{
	return _wordNet->isPerson(word) ||
		   _significantHeadWords->lookup(lowercaseSymbol(word)) ||
		   _significantHeadWords->lookup(_wordNet->stem_noun(word));
}

bool EnglishDescOracle::isNounPhrase(ChartEntry *entry, Symbol chain) const {
	Symbol category = entry->constituentCategory;
	return category == EnglishSTags::NP ||
		   category == EnglishSTags::NPA ||
		   _npChains->lookup(chain);
}

Symbol EnglishDescOracle::lowercaseSymbol(Symbol s) const
{
	wstring str = s.to_string();
	wstring::size_type length = str.length();
    for (unsigned i = 0; i < length; ++i) {
        str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
}
