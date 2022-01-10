// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Arabic/parse/ar_DescOracle.h"
#include "Arabic/parse/ar_STags.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8Token.h"
//Notes: Currently there is not an NP/NPA distinction in Arabic, only look for NP, =NP
ArabicDescOracle::ArabicDescOracle(KernelTable* kernelTable, ExtensionTable* extensionTable) {
	
	_npChains = _new SymbolHash(1000);

	ExtensionTable::iterator i;
	for (i = extensionTable->table->begin(); i != extensionTable->table->end(); ++i) {
		int numExtensions = (*i).second.length;
		BridgeExtension* be = (*i).second.data;
		for (int j = 0; j < numExtensions; j++) {
			std::wstring s_string = be[j].modifierChain.to_string();
			if (s_string.find(L"=NP") != std::wstring::npos || 	s_string.find(L"NP") == 0 )
				_npChains->add(be[j].modifierChain);
		}
	}
	KernelTable::iterator k;
	for (k = kernelTable->table->begin(); k != kernelTable->table->end(); ++k) {
		int numKernels = (*k).second.length;
		BridgeKernel* bk = (*k).second.data;
		for (int j = 0; j < numKernels; j++) {
			std::wstring s_string = bk[j].headChain.to_string();
			if (s_string.find(L"=NP") != std::wstring::npos || 	s_string.find(L"NP") == 0 )
				_npChains->add(bk[j].headChain);
			s_string = bk[j].modifierChain.to_string();
			if (s_string.find(L"=NP") != std::wstring::npos || 	s_string.find(L"NP") == 0 )
				_npChains->add(bk[j].modifierChain);
		}
	}
}

bool ArabicDescOracle::isPossibleDescriptor(ChartEntry *entry, Symbol chain) const{
//NPPOS is not a specific structure in Arabic 
//HeadIsSignficant will only be set true for names.  Appositives are (NP (NP )(NP ))
//Make all NPs possible descriptors- until develop a list of signicant names....
	if (isNounPhrase(entry, chain)) 
		return true;
	else 
		return false;
}
bool ArabicDescOracle::isNounPhrase(ChartEntry *entry, Symbol chain) const{
	if(entry->constituentCategory == ArabicSTags::NP || _npChains->lookup(chain) )
		return true;
	else 
		return false;

}
