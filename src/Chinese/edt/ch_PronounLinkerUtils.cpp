// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/edt/ch_PronounLinkerUtils.h"
#include "Generic/common/WordConstants.h"
#include "Generic/common/SymbolConstants.h"
#include "Chinese/parse/ch_STags.h"
#include "Generic/theories/SynNode.h"

#ifdef _WIN32
	#define swprintf _snwprintf
#endif


Symbol ChinesePronounLinkerUtils::augmentIfPOS(Symbol type, const SynNode *node) {
	if(WordConstants::isPossessivePronoun(node->getHeadWord())) {
		Symbol pair[] = {type, Symbol(L"POS")};
		return combineSymbols(pair, 2);
	}
	return type;
}

Symbol ChinesePronounLinkerUtils::combineSymbols(Symbol symArray[], int nSymArray, bool use_delimiter) {
	wchar_t combined[256] = L"";
	const wchar_t delim[] = L".";
	for (int i=0; i<nSymArray; i++) {
		wcscat(combined, symArray[i].to_string());
		if (use_delimiter&& i!=nSymArray-1) wcscat(combined, delim);
	}
	return Symbol(combined);
}

Symbol ChinesePronounLinkerUtils::getNormDistSymbol(int distance) {
	//first normalize the distance
	if(distance > 20) distance = 12;
	else if (distance > 10) distance = 11;
	//now concatenate an "H" with the number
	wchar_t number[10];
	swprintf(number, 10, L"H%d", distance);
	return Symbol(number);
}


// returns parent's headword. If said headword is a copula, we tack on the parent's head's first postmodifier

Symbol ChinesePronounLinkerUtils::getAugmentedParentHeadWord(const SynNode *node) {
	if(node == NULL || node->getParent() == NULL)
		return SymbolConstants::nullSymbol;
	
	Symbol parheadtag = node->getParent()->getHeadPreterm()->getTag();
	Symbol result = node->getParent()->getHeadWord();
	if(parheadtag == ChineseSTags::VC) {
		//append first postmodifier if it exists
		const SynNode *parentHead = node->getParent()->getHead();
		int firstPostmodIndex = parentHead->getHeadIndex()+1;
		if(firstPostmodIndex < parentHead->getNChildren()) {
			Symbol pair[] = {result, parentHead->getChild(firstPostmodIndex)->getHeadWord()};
			result = combineSymbols(pair, 2);
		}
	}
	return result;
}


Symbol ChinesePronounLinkerUtils::getAugmentedParentHeadTag(const SynNode *node) {
	if(node == NULL || node->getParent() == NULL)
		return SymbolConstants::nullSymbol;
	
	Symbol parheadtag = node->getParent()->getHeadPreterm()->getTag();
	Symbol partag = node->getParent()->getHead()->getTag();
	if(parheadtag == ChineseSTags::VC) {
		//append first postmodifier if it exists
		const SynNode *parent = node->getParent();
		int headIndex = parent->getHeadIndex();
		if(headIndex < parent->getNChildren()-1) {
			Symbol pair[] = {partag, parent->getChild(headIndex+1)->getHead()->getTag()};
			partag = combineSymbols(pair,2);
		}
	}
	return partag;
}
