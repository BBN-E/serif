// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Arabic/common/ar_SymbolUtilities.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "Arabic/common/ar_WordConstants.h"
#include <sstream>

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

Symbol ArabicSymbolUtilities::stemDescriptor(Symbol word) {
	Symbol w1 = ArabicWordConstants::removeAl(word);
	Symbol w2 = ArabicWordConstants::getNationalityStemVariant(w1);
	return w1;
}

Symbol ArabicSymbolUtilities::getFullNameSymbol(const SynNode *nameNode) {
	std::wostringstream nameStr;
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		nameStr << word.to_string();
		if (i < nameNode->getNChildren() - 1) 
			nameStr << L" ";
	}			
	return Symbol(nameStr.str().c_str());
}
Symbol ArabicSymbolUtilities::getStemmedFullName(const SynNode *nameNode) {
	//only allow this to work for 1 word names
	std::wostringstream nameStr;
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		Symbol w1 = ArabicWordConstants::removeAl(word);
		//Symbol w2 = ArabicWordConstants::getNationalityStemVariant(w1);
		nameStr << w1.to_string();
		if (i < nameNode->getNChildren() - 1) 
			nameStr << L" ";
	}			
	return Symbol(nameStr.str().c_str());
}

int ArabicSymbolUtilities::getStemVariants(Symbol word, Symbol* variants, int MAX_RESULTS){
	return ArabicWordConstants::getPossibleStems(word, variants, MAX_RESULTS);
}

Symbol ArabicSymbolUtilities::getWordWithoutAl(Symbol word){ 
	return ArabicWordConstants::removeAl(word);
};




