// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "English/edt/en_NameLinkFunctions.h"
#include "Generic/edt/AcronymMaker.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/theories/EntityType.h"

bool EnglishNameLinkFunctions::populateAcronyms(const Mention *mention, EntityType type) {
	Symbol words[32], resolvedWords[32];
	int nWords = mention->getHead()->getTerminalSymbols(words, 32);
	int nResolvedWords;
	Symbol acronyms[16];
	if(!type.matchesORG() && !type.matchesGPE())
		return false;
	int nAcronyms = AcronymMaker::getSingleton().generateAcronyms(words, nWords, acronyms, 16);

	nResolvedWords = AbbrevTable::resolveSymbols(words, nWords, resolvedWords, 32);

	for(int i=0; i<nAcronyms; i++)
		AbbrevTable::add(&acronyms[i], 1, resolvedWords, nResolvedWords);
	return (nAcronyms > 0);
	
}

void EnglishNameLinkFunctions::recomputeCounts(CountsTable &inTable, 
										CountsTable &outTable, 
										int &outTotalCount)
{
	outTotalCount = 0;
	outTable.cleanup();
	for(CountsTable::iterator iterator = inTable.begin(); iterator!=inTable.end(); ++iterator) {
		Symbol unresolved, resolved[16];
		int nResolved;
		unresolved = iterator.value().first;
		int thisCount = iterator.value().second;
		nResolved = AbbrevTable::resolveSymbols(&unresolved, 1, resolved, 16);
		for(int i=0; i<nResolved; i++) {
			outTable.add(resolved[i], thisCount);
			outTotalCount += thisCount;
		}
	}
}
