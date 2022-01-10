// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Chinese/common/ch_SymbolUtilities.h"

#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"

Symbol ChineseSymbolUtilities::lowercaseSymbol(Symbol sym) { 
	std::wstring str = sym.to_string();
	std::wstring::size_type length = str.length();
	for (size_t i = 0; i < length; ++i) {
		str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
}
Symbol ChineseSymbolUtilities::getFullNameSymbol(const SynNode *nameNode) {
	std::wstring str = L"";
	int offset = 0;
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		str += lowercaseSymbol(word).to_string();
	}			
	return Symbol(str.c_str());
}
bool ChineseSymbolUtilities::isClusterableWord(Symbol word) {
	const wchar_t* wordstr = word.to_string();
	int len = static_cast<int>(wcslen(wordstr));
	bool foundlet = false;
	for (int i = 0; i < len; i++) {
		// Note: iswalpha doesn't behave consistently across all platforms
		if (iswcntrl(wordstr[i]) == 0 && iswdigit(wordstr[i]) == 0 && 
			iswpunct(wordstr[i]) == 0 && iswspace(wordstr[i]) == 0) 
		{
			foundlet = true;
		}
	}
	return foundlet;
}
