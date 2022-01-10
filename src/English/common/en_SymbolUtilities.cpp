// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "English/common/en_SymbolUtilities.h"

#include "Generic/common/Symbol.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Mention.h"
#include "Generic/theories/MentionSet.h"
#include "English/edt/en_PreLinker.h"
#include "Generic/wordnet/xx_WordNet.h"
#include "Generic/common/WordConstants.h"
#include "English/parse/en_STags.h"
#include "Generic/parse/LanguageSpecificFunctions.h"
#include "English/common/en_WordConstants.h"


Symbol EnglishSymbolUtilities::stemDescriptor(Symbol word) {
	return WordNet::getInstance()->stem_noun(word);
}

Symbol EnglishSymbolUtilities::stemWord(Symbol word, Symbol pos) {
	if (LanguageSpecificFunctions::isNPtypePOStag(pos)) {
		return WordNet::getInstance()->stem_noun(word);
	} else if (pos == EnglishSTags::VB ||
		pos == EnglishSTags::VBD ||
		pos == EnglishSTags::VBG ||
		pos == EnglishSTags::VBN ||
		pos == EnglishSTags::VBP ||
		pos == EnglishSTags::VBZ) 
	{
		return WordNet::getInstance()->stem_verb(word);
	} else return word;
}

int EnglishSymbolUtilities::fillWordNetOffsets(Symbol word, int *offsets, int MAX_OFFSETS) {
	return fillWordNetOffsets(word, Symbol(), offsets, MAX_OFFSETS);
}

// original function
/*
int EnglishSymbolUtilities::fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS) {
	int n_hypernyms = 0;		
	while (n_hypernyms < MAX_OFFSETS) {
        int offset = WordNet::getInstance()->getNthHypernymClass(word, pos, n_hypernyms);
        if (offset == 0)
			break;
		offsets[n_hypernyms++] = offset;
	}
	return n_hypernyms;
}
*/

// new implementation for speed-up WordNet access
// at present, this function is often called by   getHypernymOffsets(word, symbol(), offsets, MAX_OFFSETS)
int EnglishSymbolUtilities::fillWordNetOffsets(Symbol word, Symbol pos, int *offsets, int MAX_OFFSETS) {
	return WordNet::getInstance()->getHypernymOffsets(word, pos, offsets, MAX_OFFSETS);
}

int EnglishSymbolUtilities::getHypernyms(Symbol word, Symbol *results, int MAX_RESULTS) {
	return WordNet::getInstance()->getHypernyms(word, results, MAX_RESULTS);
}

Symbol EnglishSymbolUtilities::getFullNameSymbol(const SynNode *nameNode) {
	std::wstring str = L"";
	int offset = 0;
	for (int i = 0; i < nameNode->getNChildren(); i++) {
		Symbol word = nameNode->getChild(i)->getHeadWord();
		str += word.to_string();
		if (i < nameNode->getNChildren() - 1)
			str += L" ";
	}			
	return Symbol(str.c_str());
}

float EnglishSymbolUtilities::getDescAdjScore(MentionSet* mentionSet) {
	float score = 0;
	for (int i = 0; i < mentionSet->getNMentions(); i++) {
		const Mention *mention = mentionSet->getMention(i);
		if (mention->getMentionType() == Mention::APPO &&
			mention->getEntityType().isRecognized()) {
			score += 10000;
		}
		if (EnglishPreLinker::getTitle(mentionSet, mention)) {
			score += 10000;
		}
		if (mention->getMentionType() == Mention::LIST) {
			// Constructions like (X and Y)'s FOO are likely to be misparsed
			// We prefer (X) and (Y's FOO)... in a random examination of cases parsed as (X and Y)'s FOO, 75% were wrong
			if (mention->getNode()->getParent() != 0 && mention->getNode()->getParent()->getTag() == EnglishSTags::NPPOS) {
				score -= 20000;
			}
		}
	}

	// This next step is designed to encourage the system to
	// correctly parse constructions such as "DAVAO, Philippines, March 4"
	// It should prefer (NPA (NPP-DAVAO , NPP-Philippines) , (NPA March 4))
	//               to (NPA (NPP-DAVAO) , (NPP-Philippines) , (NPA March 4))
	if (mentionSet->getSentenceNumber() == 0) {
		for (int i = 0; i < mentionSet->getNMentions(); i++) {
			const Mention *mention = mentionSet->getMention(i);
			if (mention->getMentionType() == Mention::NAME &&
				mention->getEntityType().matchesGPE()) {
					const SynNode *node = mention->getNode();
					if (node->getNChildren() == 3 &&
						node->getChild(0)->getTag() == EnglishSTags::NPP &&
						node->getChild(1)->getTag() == EnglishSTags::COMMA &&
						node->getChild(2)->getTag() == EnglishSTags::NPP) {
							score += 10000;						
					}
			}
		}
	}
	
	return score;
}
Symbol EnglishSymbolUtilities::lowercaseSymbol(Symbol sym) {
	std::wstring str = sym.to_string();
	std::wstring::size_type length = str.length();
	for (size_t i = 0; i < length; ++i) {
		str[i] = towlower(str[i]);
	}
	return Symbol(str.c_str());
}

//mrf 2007, sometimes Arabic names start with Al, even in English.  We
//don't always tokenize correctly, try this feature
Symbol EnglishSymbolUtilities::getWordWithoutAl(Symbol word){
	const wchar_t* word_str = word.to_string();
	size_t word_len = wcslen(word_str);
	if(word_len < 4) return word;
	if(word_len > 199) return word;
	if((word_str[0] == L'A') || (word_str[0] == L'a')){
		if((word_str[1] == L'L') || (word_str[1] == L'l')){
			int start = 2;
			if(word_str[2] == L'-'){
				start = 3;
			}
			wchar_t word_no_al[200];
			int curr_char = 0;
			for(size_t i = start; i< word_len; i++){
				word_no_al[curr_char++] = word_str[i];
			}
			word_no_al[curr_char] = L'\0';
			return Symbol(word_no_al);
		}
	}
	return word;
}
	
bool EnglishSymbolUtilities::isPluralMention(const Mention* ment){
	if(ment->getMentionType() == Mention::PRON){
		Symbol hw = ment->getNode()->getHeadWord();			
		if(hw == EnglishWordConstants::THEIR) return true;
		if(hw == EnglishWordConstants::THEY) return true;
		if(hw == EnglishWordConstants::THEM) return true;
		if(hw == EnglishWordConstants::OUR) return true;
		if(hw == EnglishWordConstants::US) return true;
		if(hw == EnglishWordConstants::WE) return true;
	}
	if(ment->getNode()->getHeadPreterm()->getTag() == EnglishSTags::NNS) return true;
	else if(ment->getNode()->getHeadPreterm()->getTag() == EnglishSTags::NNPS) return true;
	return false;
}
