// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/common/xx_SymbolUtilities.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"


float GenericSymbolUtilities::getDescAdjScore(MentionSet* mentionSet) { 
	float score = 0;
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getMentionType() == Mention::APPO &&
			mention->getEntityType().isRecognized())
			score += 1000;		
	}	
	return score;
}

bool GenericSymbolUtilities::isBracket(Symbol word) {
	static const Symbol::SymbolGroup brackets = Symbol::makeSymbolGroup
		(L"-LRB- -RRB- -LCB- -RCB- -LDB- -RDB-");
	return (brackets.find(word) != brackets.end());
}

bool GenericSymbolUtilities::isClusterableWord(Symbol word) {
	const wchar_t* wordstr = word.to_string();
	int len = static_cast<int>(wcslen(wordstr));
	bool foundlet = false;
	for (int i = 0; i < len; i++) {
		if (iswalpha(wordstr[i]) != 0) {
			foundlet = true;
		}
	}
	return foundlet;
}


Symbol GenericSymbolUtilities::getFullNameSymbol(const SynNode *nameNode) {
	std::wostringstream nameStr;
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		nameStr << word.to_string();
		if (i < nameNode->getNChildren() - 1) 
			nameStr << L" ";
	}			
	return Symbol(nameStr.str().c_str());
}

