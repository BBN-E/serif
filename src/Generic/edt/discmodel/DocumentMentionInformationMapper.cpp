// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolArray.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SymbolUtilities.h"
#include "Generic/edt/discmodel/DocumentMentionInformationMapper.h"
#include "Generic/edt/AbbrevTable.h"
#include "Generic/edt/NameLinkFunctions.h"
#include "Generic/edt/Guesser.h"
#include "Generic/theories/SynNode.h"
#include "Generic/common/version.h"
#include "Generic/common/WordConstants.h"



//#include <xstring>

Symbol DASH(L"-");
Symbol APOSTROF(L"'");
Symbol S(L"'s");
Symbol AL(L"al");
Symbol LRB(L"-lrb-");
Symbol RRB(L"-rrb-");
Symbol A(L"a");
Symbol AN(L"an");
Symbol THE(L"the");
Symbol DOT(L".");
Symbol COMMA(L",");
Symbol SEMICOLON(L";");
Symbol TAG(L"`");


DocumentMentionInformationMapper::DocumentMentionInformationMapper() : 
_mentionToAbbrev(300), _mentionToHW(300), 
_mentionToWords(300), _mentionToPremod(300), 
_mentionToPostmod(300), _mentionToWordsWithinBrackets(300), 
_mentionToGender(300), _mentionToNumber(300) {
	// initialize the AbbrevTable
	AbbrevTable::initialize();
}

namespace { 
	void clearMentionSymArrayMap(MentionSymArrayMap &map) {
		MentionSymArrayMap::iterator iter;
		// Clean abberviations
		for (iter = map.begin(); iter != map.end(); ++iter)
		{
			SymbolArray *oldValue = (*iter).second;
			(*iter).second = NULL;
			delete oldValue;
		}
		map.clear();
	}
}

void DocumentMentionInformationMapper::cleanUpAfterDocument() {
	AbbrevTable::cleanUpAfterDocument();
	clearMentionSymArrayMap(_mentionToAbbrev);
	clearMentionSymArrayMap(_mentionToHW);
	clearMentionSymArrayMap(_mentionToWords);
	clearMentionSymArrayMap(_mentionToPremod);
	clearMentionSymArrayMap(_mentionToPostmod);
	clearMentionSymArrayMap(_mentionToWordsWithinBrackets);
	_mentionToGender.clear();
	_mentionToNumber.clear();
}




void DocumentMentionInformationMapper::addMentionInformation(const Mention *mention, TokenSequence *tokenSequence){
	Symbol origHeadWords[32], mentWords[32], notInHeadWords[32];
	Symbol headWords[32], resolvedWords[32], lexicalWords[32], wordsWithinBrackets[32];

	MentionUID mentUID = mention->getUID();

	const SynNode *node = mention->getNode();
	int nMentWords = node->getTerminalSymbols(mentWords,32);
	// Insert mention words into the table
	SymbolArray *mentArray = _new SymbolArray(mentWords, nMentWords);
	if(_mentionToWords.get(mentUID) != NULL) {
		SymbolArray *oldValue = _mentionToWords[mentUID];
		delete oldValue;
	}
	_mentionToWords[mentUID] = mentArray;


	// deal with the mention Head Words
	const SynNode *hNode = mention->getHead();
	if (hNode==NULL)
		return;
	int nOrigHeadWords = hNode->getTerminalSymbols(origHeadWords, 32);
	int nWordsWithinBrackets = -1;
	int nHeadWords = cleanWords(origHeadWords, nOrigHeadWords, headWords, 32, wordsWithinBrackets, &nWordsWithinBrackets);

	// print a warning
	if(nHeadWords==0) {
		std::ostringstream ostr;
		ostr<<"ment words: [";
		for (int i=0; i<nMentWords-1; i++)
			ostr<<mentWords[i]<<" ";
		if(nMentWords>=1)
			ostr<<mentWords[nMentWords-1].to_string()<<L"]";
		ostr<<std::endl;

		ostr<<"results: [";
		for (int i=0; i<nOrigHeadWords-1; i++) {
			ostr<<L"("<<origHeadWords[i]<<L") ";
		}
		if(nOrigHeadWords>=1)
			ostr<<origHeadWords[nOrigHeadWords-1].to_debug_string()<<"]";
		ostr<<" reduced to 0 tokens. This may cause a problem.\n";
		SessionLogger::warn("document_mention_mapper") << ostr.str();
	}
///*
//		wcerr<<"headwords: [";
//		for (int i=0; i<nOrigHeadWords-1; i++) {
//			std::wcerr<<L"("<<origHeadWords[i]<<L") ";
//		}
//		if(nOrigHeadWords>=1)
//			std::wcerr<<L"("<<origHeadWords[nOrigHeadWords-1].to_string()<<")]";
//		std::wcerr<<" --> [";
//		for (int i=0; i<nHeadWords-1; i++) {
//			std::wcerr<<L"("<<headWords[i]<<L") ";
//		}
//		if(nHeadWords>=1)
//			std::wcerr<<L"("<<headWords[nHeadWords-1].to_string()<<")]";		
//		std::wcerr<<"\n";
//*/

	// Insert HeadWords to the table
	SymbolArray *hwArray = _new SymbolArray(headWords, nHeadWords);
	if(_mentionToHW.get(mentUID) != NULL) {
		SymbolArray *oldValue = _mentionToHW[mentUID];
		delete oldValue;
	}
	_mentionToHW[mentUID] = hwArray;

	// Insert words in bracket into the table
	if (nWordsWithinBrackets>0) {
		SymbolArray *wordsWithinBracketsArray = _new SymbolArray(wordsWithinBrackets, nWordsWithinBrackets);
		if(_mentionToWordsWithinBrackets.get(mentUID) != NULL) {
			SymbolArray *oldValue = _mentionToWordsWithinBrackets[mentUID];
			delete oldValue;
		}
		_mentionToWordsWithinBrackets[mentUID] = wordsWithinBracketsArray;
	}
	
	// Map Gender
	Symbol gender = Guesser::guessGender(node, mention);
	_mentionToGender[mentUID] = gender;

	// Map Number
	Symbol number = Guesser::guessNumber(node, mention);
	_mentionToNumber[mentUID] = number;

	// Insert Abbreviations & words not in the mention's head
	int nResolvedWords  = AbbrevTable::resolveSymbols(headWords, nHeadWords, resolvedWords, 32);
	int nLexicalWords = NameLinkFunctions::getLexicalItems(resolvedWords, nResolvedWords, lexicalWords, 32);
	SymbolArray *abbrevArray = _new SymbolArray(lexicalWords, nLexicalWords);
	if(_mentionToAbbrev.get(mentUID) != NULL) {
		SymbolArray *oldValue = _mentionToAbbrev[mentUID];
		delete oldValue;
	}
	_mentionToAbbrev[mentUID] = abbrevArray;

	EntityType linkType = mention->getEntityType();
	// add acronyms to the document abbrev map
	if(linkType.isDetermined())
		NameLinkFunctions::populateAcronyms(mention, linkType);

	// currently tokenSequence is only available when training with State Files
	int mentStartToken = node->getStartToken();
	int mentEndToken = node->getEndToken();
	int mentHeadStartToken = hNode->getStartToken();
	int mentHeadEndToken = hNode->getEndToken();
	if(tokenSequence !=NULL) { 
		// insert premods (words not in the head before the head)
		int n=0;
		if (mentStartToken < mentHeadStartToken){
			for (int i= mentStartToken; i<mentHeadStartToken; i++) {
				Symbol nih = tokenSequence->getToken(i)->getSymbol();
				nih = SymbolUtilities::lowercaseSymbol(nih);
				if (!WordConstants::isPronoun(nih)
					//#ifdef ENGLISH_LANGUAGE
					//					&& !WordConstants::isSingleCharacter(nih) 
					//					&& !WordConstants::isPunctuation(nih) && !WordConstants::isUnknownRelationReportedPreposition(nih)
					//					&& !WordConstants::isCopula(nih) 
					//#endif
					&& !nih.is_null()
					&& nih != S
					&& nih != THE && nih != A && nih != DOT
					&& n<32)
				{
					if (!SerifVersion::isEnglish()) {
						notInHeadWords[n++] = nih;
					} else if ( 
						!WordConstants::isSingleCharacter(nih) &&
						!WordConstants::isPunctuation(nih) &&
						!WordConstants::isUnknownRelationReportedPreposition(nih) &&
						!WordConstants::isCopula(nih))
					{
						notInHeadWords[n++] = nih;
					}

				}

			}
			if(n>0){
				SymbolArray *notInHeadArray = _new SymbolArray(notInHeadWords, n);
				if(_mentionToPremod.get(mentUID) != NULL) {
					SymbolArray *oldValue = _mentionToPremod[mentUID];
					delete oldValue;
				}
				_mentionToPremod[mentUID] = notInHeadArray;
			}
		}

		// insert premods (words not in the head before the head)
		n=0;
		if (mentEndToken > mentHeadEndToken){
			for (int i= mentHeadEndToken+1; i<=mentEndToken; i++) {
				Symbol nih = tokenSequence->getToken(i)->getSymbol();
				nih = SymbolUtilities::lowercaseSymbol(nih);
				if (!WordConstants::isPronoun(nih) 
					//#ifdef ENGLISH_LANGUAGE
					//					&& !WordConstants::isSingleCharacter(nih) 
					//					&& !WordConstants::isPunctuation(nih) && !WordConstants::isUnknownRelationReportedPreposition(nih)
					//					&& !WordConstants::isCopula(nih)
					//#endif
					&& !nih.is_null()
					&& nih != S
					&& nih != THE && nih != A && nih != AN && nih != DOT
					&& n<32)
				{
					if (!SerifVersion::isEnglish()) {
						notInHeadWords[n++] = nih;
					} else if ( 
						!WordConstants::isSingleCharacter(nih) &&
						!WordConstants::isPunctuation(nih) &&
						!WordConstants::isUnknownRelationReportedPreposition(nih) &&
						!WordConstants::isCopula(nih))
					{
						notInHeadWords[n++] = nih;
					}
				}
			}
			if(n>0){
				SymbolArray *notInHeadArray = _new SymbolArray(notInHeadWords, n);
				if(_mentionToPostmod.get(mentUID) != NULL) {
					SymbolArray *oldValue = _mentionToPostmod[mentUID];
					delete oldValue;
				}
				_mentionToPostmod[mentUID] = notInHeadArray;
			}
		}
	}
}


int DocumentMentionInformationMapper::cleanWords(Symbol words[], int nWords
		, Symbol results[], int maxNResults, Symbol wordsInBrackets[], int *nWordsInBrackets)
{
	int nResults = nWords;
	for (int i=0; i<nResults; i++)
		results[i] = words[i];

	// clean the "'s", "-", "'" "-LRB-" "." & "al" tokens from the name
	for (int i=0; i<nResults; i++) {
		if (results[i]==DASH || results[i]==APOSTROF || results[i]==S 
				|| results[i]==DOT || results[i]==COMMA || results[i]==SEMICOLON || results[i]==TAG) {
			if(nResults>1) { // to prevent empty heads
				for(int j=i; j<nResults-1;j++)
					results[j] = results[j+1];
				nResults--;
				i--;
			}
		}
	}

	
	for (int i=0; i<nResults; i++) {
		if (results[i]==LRB || results[i]==RRB)
			continue;
		std::wstring str = results[i].to_string();
		if(str[0]==L'-') //  remove beginning -xxx
			str = str.substr(1);
		size_t pos = str.find_first_of(L'-');
		while(pos != std::wstring::npos && pos>0) {
			str.erase(pos,1);
			pos = str.find_first_of(L'-');
		}
		results[i] = Symbol(str.c_str());
	}

	for (int i=0; i<nResults; i++) {
		std::wstring str = results[i].to_string();
		if(str[0]==L'\'') //  remove beginning -xxx
			str = str.substr(1);
		size_t pos = str.find_first_of(L'\'');
		while(pos != std::wstring::npos && pos>0) {
			str.erase(pos,1);
			pos = str.find_first_of(L'\'');
		}
		results[i] = Symbol(str.c_str());
	}


	// clean the "al" tokens from the name (second time after removing '-')
	for (int i=0; i<nResults; i++) {
		if (results[i]==AL) {
			if(nResults>1 && i<nResults-1 && results[i+1]!=Symbol(L"gore")) { // deal with "al gore"
				for(int j=i; j<nResults-1;j++)
					results[j] = results[j+1];
				nResults--;
				i--;
			}
		}
	}

	// deal with text between brackets and get rid of the brackets
	*nWordsInBrackets = 0;
	bool in_brackets = false;
	for (int i=0; i<nResults; i++) {
		Symbol word = results[i];
		if (word==LRB) {
			if(nResults>1) { // to prevent empty heads
				for(int j=i; j<nResults-1;j++)
					results[j] = results[j+1];
				nResults--;
				i--;
			}
			in_brackets = true;
		} else if(word==RRB) {
			if(nResults>1) { // to prevent empty heads
				for(int j=i; j<nResults-1;j++)
					results[j] = results[j+1];
				nResults--;
				i--;
			}
			in_brackets = false;
		} else if(in_brackets) {
//			if(nResults>1) { // to prevent empty heads
//				for(int j=i; j<nResults-1;j++)
//					results[j] = results[j+1];
//				nResults--;
//				i--;
//			}
			wordsInBrackets[(*nWordsInBrackets)++] = word;
		}
	}

	return nResults;
}
